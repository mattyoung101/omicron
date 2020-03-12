#include "computer_vision.hpp"
#include <opencv2/core/utility.hpp>
#include "log/log.h"
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/gapi.hpp>
#include <opencv2/gapi/core.hpp>
#include <opencv2/gapi/imgproc.hpp>
#include <opencv2/core/cuda.hpp>
#include <map>
#include <unistd.h>
#include "defines.h"
#include "utils.h"
#include "remote_debug.h"
#include "rpa_queue.h"
#include "movavg.h"
#include "localisation.h"
// yeah this is bad practice, what are you gonna do?
using namespace cv;
using namespace std;

// This file handles all the OpenCV tasks in C++ code, which is then called by the C code in the rest of the project,
// as OpenCV dropped support for the legacy C API.
// I also sincerely apologise in advance for the somewhat hideous mix of C-style code and modern C++-style code, I didn't have
// the time or energy to learn proper modern C++ for just this file in a majority C project.

/** A detected field object and its associated information */
typedef struct {
    bool exists;
    RDPointF centroid;
    RDRect boundingBox;
    Mat threshMask;
} object_result_t;

static pthread_t cvThread = {0};
static pthread_t fpsCounterThread = {0};
static _Atomic int32_t lastFpsMeasurement = 0;
static Rect cropRect;
static movavg_t *fpsAvg;
static int32_t errorCount = 0;

/**
 * Uses the specified threshold values to threshold the given frame and detect the specified object on the field.
 * This is in a separate function so that we can just invoke it for each object, likes "ball", "lines", etc.
 * @param frame the thresholded object to be processed
 * @param objectId what the object is
 * @param realWidth the real width of the (potentially cropped) frame, before downscaling
 * @param realHeight the real size of the (potentially cropped) frame, before downscaling
 */
static object_result_t process_object(const Mat& thresholded, field_objects_t objectId, int32_t realWidth, int32_t realHeight){
    // shortcut just for lines
    if (objectId == OBJ_LINES){
        object_result_t result = {}; // NOLINT
        result.exists = true;
        result.centroid = {0.0, 0.0};
        result.boundingBox = {};
        result.threshMask = thresholded;
        return result;
    }

    Mat labels, stats, centroids, threshScaled;
    int nLabels = connectedComponentsWithStats(thresholded, labels, stats, centroids);

    // find the biggest blob, skipping id 0 which is the background
    vector<int> sortedLabels;
    for (int label = 1; label < nLabels; label++){
        sortedLabels.push_back(label);
    }
    sort(sortedLabels.begin(), sortedLabels.end(), [=](const int &a, const int &b){
        int aSize = stats.at<int>(a, CC_STAT_AREA);
        int bSize = stats.at<int>(b, CC_STAT_AREA);
        return aSize < bSize;
    });

    auto blobExists = !sortedLabels.empty();
    auto largestId = !blobExists ? -1 : sortedLabels.back();
    auto largestCentroid = !blobExists ? Point2f(0.0, 0.0) : Point2f(centroids.at<double>(largestId, 0),
                                                             centroids.at<double>(largestId, 1));
    RDRect objRect = {};
    if (blobExists) {
        int rectX = stats.at<int>(largestId, CC_STAT_LEFT);
        int rectY = stats.at<int>(largestId, CC_STAT_TOP);
        int rectW = stats.at<int>(largestId, CC_STAT_WIDTH);
        int rectH = stats.at<int>(largestId, CC_STAT_HEIGHT);
        objRect = {rectX, rectY, rectW, rectH};
    }

    // rescale coordinates if the image was downscaled
    if (VISION_IS_RESCALED){
        largestCentroid.x /= VISION_SCALE_FACTOR;
        largestCentroid.y /= VISION_SCALE_FACTOR;
        objRect.x /= VISION_SCALE_FACTOR;
        objRect.y /= VISION_SCALE_FACTOR;
        objRect.width /= VISION_SCALE_FACTOR;
        objRect.height /= VISION_SCALE_FACTOR;

        // also rescale image if remote debug is enabled, otherwise don't bother for performance sake
        if (remote_debug_is_connected() && blobExists){
            resize(thresholded, threshScaled, Size(realWidth, realHeight), INTER_NEAREST);
        }
    }

    // translate coordinates if cropping is enabled
#if VISION_CROP_ENABLED
    objRect.x += cropRect.x;
    objRect.y += cropRect.y;
    largestCentroid.x += cropRect.x;
    largestCentroid.y += cropRect.y;
#endif

    object_result_t result = {}; // NOLINT
    result.exists = blobExists;
    result.centroid = {largestCentroid.x, largestCentroid.y};
    result.boundingBox = objRect;
    if (VISION_IS_RESCALED){
        result.threshMask = threshScaled;
    } else {
        result.threshMask = thresholded;
    }

    return result;
}

/** this task runs all the computer vision for Omicam, using OpenCV */
static auto cv_thread(void *arg) -> void *{
    uint32_t frameCounter = 0;
    log_trace("Vision thread started");

#if BUILD_TARGET == BUILD_TARGET_PC
    log_trace("Build target is PC, using test data");
    Mat ogFrame = imread("../test_data/field4.png");
    if (ogFrame.cols <= 0 || ogFrame.rows <= 0){
        log_error("Unable to load test image! Please check the path is correct. Cannot continue.");
        return nullptr;
    }
    VideoCapture cap("../test_data/test_footage_2.m4v");
    if (!cap.isOpened()) {
        log_error("Failed to load OpenCV test video, cannot continue");
        fflush(stdout);
        fflush(stderr);
        exit(EXIT_FAILURE);
    }
#else
    log_trace("Build target is SBC, initialising VideoCapture");
    // TODO make the width be "videoWidth" and the height also be "videoHeight"
    VideoCapture cap("v4l2src device=/dev/video0 ! image/jpeg, width=1280, height=720 ! jpegdec ! videoconvert"
                     " ! appsink drop=true", CAP_GSTREAMER);
    if (!cap.isOpened()){
        log_error("Failed to open OpenCV capture device in SBC build: impossible to continue running Omicam.");
        fflush(stdout);
        fflush(stderr);
        exit(EXIT_FAILURE); // TODO should we exit with EXIT_SUCCESS so that we don't keep being restarted by run.py?
    }
#endif
    auto fps = ROUND2INT(cap.get(CAP_PROP_FPS));
    log_debug("Video capture initialised, API: %s, framerate: %d", cap.getBackendName().c_str(), fps);

    // calculate cropped frame size
    Mat junk(videoHeight, videoWidth, CV_8UC1);
#if VISION_CROP_ENABLED
    junk = junk(cropRect);
#endif
    resize(junk, junk, Size(), VISION_SCALE_FACTOR, VISION_SCALE_FACTOR);
    log_info("Frame size: %dx%d (cropping enabled: %s)", videoWidth, videoHeight, VISION_CROP_ENABLED ? "YES" : "NO");
    log_info("Scaled frame size: %dx%d (scale factor: %.2f)", junk.cols, junk.rows, VISION_SCALE_FACTOR);

    // generate the mirror mask always (in case it's changed later in the ini)
    Mat mirrorMask = Mat(cropRect.height, cropRect.width, CV_8UC1, Scalar(0));
    circle(mirrorMask, Point(mirrorMask.cols / 2, mirrorMask.rows / 2), visionMirrorRadius,
            Scalar(255, 255, 255),FILLED);

#if VISION_APPLY_CLAHE
    auto clahe = createCLAHE();
#endif

    while (true){
        // handle threading crap before beginning with the vision
        pthread_testcancel();
        pthread_mutex_lock(&sleepMutex);
        while (sleeping){
            pthread_cond_wait(&sleepCond, &sleepMutex);
        }
        pthread_mutex_unlock(&sleepMutex);
        double begin = utils_time_millis();

        Mat frame, frameScaled;
#if BUILD_TARGET == BUILD_TARGET_SBC
        cap.read(frame);
#else
        ogFrame.copyTo(frame);
#endif

        if (frame.empty()){
#if BUILD_TARGET == BUILD_TARGET_PC
            cap.set(CAP_PROP_FRAME_COUNT, 0);
            cap.set(CAP_PROP_POS_FRAMES, 0);
            cap.set(CAP_PROP_POS_AVI_RATIO, 0);
#else
            log_warn("Received empty frame from capture!");
            if (errorCount++ > 16){
                log_error("Too many empty frames, going to quit! Please make sure the camera is functioning correctly.");
                return nullptr;
            }
#endif
            continue;
        } else {
            errorCount = 0;
        }

        // pre-processing steps:
        // crop the frame if cropping is enabled
#if VISION_CROP_ENABLED
        frame = frame(Rect(cropRect));
#endif
        // apply CLAHE normalisation if requested, source: https://stackoverflow.com/a/47370615/5007892
#if VISION_APPLY_CLAHE
        Mat labImage;
        cvtColor(frame, labImage, COLOR_RGB2Lab);
        Mat labPlanes[3];
        split(labImage, labPlanes);
        clahe->apply(labPlanes[0], labPlanes[0]);
        Mat labFinal;
        merge(labPlanes, 3, labFinal);
        cvtColor(labFinal, frame, COLOR_Lab2RGB);
#endif
        // apply mirror mask if that's enabled
        if (isDrawMirrorMask) {
            Mat tmp;
            bitwise_and(frame, frame, tmp, mirrorMask);
            frame = tmp;
        }

        // downscale for goal detection (and possibly initial ball pass)
        resize(frame, frameScaled, Size(), VISION_SCALE_FACTOR, VISION_SCALE_FACTOR, INTER_NEAREST);

        // vision processing:
        // pre-calculate thresholds in parallel, make sure you get the range right, we care only about the begin
        Mat thresholded[5] = {};
        parallel_for_(Range(1, 5), [&](const Range& range){
            auto object = (field_objects_t) range.start;
            uint32_t *min, *max;

            switch (object){
                case OBJ_BALL:
                    min = (uint32_t*) minBallData;
                    max = (uint32_t*) maxBallData;
                    break;
                case OBJ_GOAL_YELLOW:
                    min = (uint32_t*) minYellowData;
                    max = (uint32_t*) maxYellowData;
                    break;
                case OBJ_GOAL_BLUE:
                    min = (uint32_t*) minBlueData;
                    max = (uint32_t*) maxBlueData;
                    break;
                case OBJ_LINES:
                    min = (uint32_t*) minLineData;
                    max = (uint32_t*) maxLineData;
                    break;
                default:
                    return;
            }

            // we re-arrange the orders of these to convert from RGB to BGR so we can skip the call to cvtColor
            Scalar minScalar = Scalar(min[2], min[1], min[0]);
            Scalar maxScalar = Scalar(max[2], max[1], max[0]);

            // decide which source image we're going to threshold from - yeah this code kinda sucks, nothing new
            if (object == OBJ_LINES){
                if (isDrawRobotMask) {
                    // make a copy to apply the robot mask on
                    Mat frameCpy;
                    frame.copyTo(frameCpy);
                    circle(frameCpy, Point(frame.cols / 2, frame.rows / 2), visionRobotMaskRadius,
                           Scalar(0, 0, 0), FILLED);
                    inRange(frameCpy, minScalar, maxScalar, thresholded[object]);
                } else {
                    inRange(frame, minScalar, maxScalar, thresholded[object]);
                }
            } else if (object == OBJ_GOAL_YELLOW || object == OBJ_GOAL_BLUE){
                inRange(frameScaled, minScalar, maxScalar, thresholded[object]);
            } else {
                inRange(frame, minScalar, maxScalar, thresholded[object]);
            }

            // morphology isn't actually very helpful here because lines are too small
//            if (object == OBJ_LINES){
//                double morphSize =1;
//                Mat element = getStructuringElement(MORPH_RECT, Size(2 * morphSize + 1, 2 * morphSize + 1),
//                        Size(morphSize, morphSize));
//                morphologyEx(thresholded[object], thresholded[object], MORPH_OPEN, element);
//            }
        }, 4);

        // process all our field objects
        int32_t realWidth = frame.cols;
        int32_t realHeight = frame.rows;
        auto ball = process_object(thresholded[OBJ_BALL], OBJ_BALL, realWidth, realHeight);
        auto yellowGoal = process_object(thresholded[OBJ_GOAL_YELLOW], OBJ_GOAL_YELLOW, realWidth, realHeight);
        auto blueGoal = process_object(thresholded[OBJ_GOAL_BLUE], OBJ_GOAL_BLUE, realWidth, realHeight);
        // note that we don't actually do CCL on the lines, process_object() is smart enough to automatically skip it
        auto lines = process_object(thresholded[OBJ_LINES], OBJ_LINES, realWidth, realHeight);

        // transmit data to ESP32
        // for each centroid, convert to cartesian (first by translating to have origin point as centre of frame)
        double cx = videoWidth / 2.0;
        double cy = videoHeight / 2.0;
        struct vec2 ballVec = {ball.centroid.x - cx, ball.centroid.y - cy};
        struct vec2 blueGoalVec = {blueGoal.centroid.x - cx, blueGoal.centroid.y - cy};
        struct vec2 yellowGoalVec = {yellowGoal.centroid.x - cx, yellowGoal.centroid.y - cy};

        // encode protocol buffer to send to ESP over UART
        ObjectData data = ObjectData_init_zero;
        data.ballExists = ball.exists;
        if (ball.exists) {
            data.ballAngle = static_cast<float>(fmod(450.0 - RAD_DEG * atan2f(ballVec.y, ballVec.x), 360.0));
            data.ballMag = (float) utils_camera_dewarp(svec2_length(ballVec));
        }malloc
        data.goalBlueExists = blueGoal.exists;
        if (blueGoal.exists) {
            data.goalBlueAngle = static_cast<float>(fmod(450.0 - RAD_DEG * atan2f(blueGoalVec.y, blueGoalVec.x), 360.0));
            data.goalBlueMag = (float) utils_camera_dewarp(svec2_length(blueGoalVec));
        }
        data.goalYellowExists = yellowGoal.exists;
        if (yellowGoal.exists) {
            data.goalYellowAngle = static_cast<float>(fmod(450.0 - RAD_DEG * atan2f(yellowGoalVec.y, yellowGoalVec.x), 360.0));
            data.goalYellowMag = (float) utils_camera_dewarp(svec2_length(yellowGoalVec));
        }
        utils_cv_transmit_data(data);

        // post data to localiser, the reason it's done so late now is because the new hybrid localisation algorithm
        // relies on a lot of data sources, including the polar vector goals which are only calculated here
        auto *localiserFrame = (uint8_t*) malloc(frame.rows * frame.cols);
        memcpy(localiserFrame, thresholded[OBJ_LINES].data, frame.rows * frame.cols);
        localiser_post(localiserFrame, frame.cols, frame.rows, {data.goalYellowAngle, data.goalYellowMag},
                {data.goalYellowAngle, data.goalYellowMag});

        // exclude debug time from fps measurement
        double elapsed = utils_time_millis() - begin;
        movavg_push(fpsAvg, elapsed);

        // dispatch frames to remote debugger
#if REMOTE_ENABLED
        if (frameCounter++ % REMOTE_FRAME_INTERVAL == 0 && remote_debug_is_connected()) {
            // we optimised out the cvtColor in the main loop so we gotta do it here instead
            // we can remove it by simply swapping the order of the threshold values to save calling BGR2RGB
            Mat debugFrame;
            cvtColor(frame, debugFrame, COLOR_BGR2RGB);

            char buf[128] = {0};
            snprintf(buf, 128, "Frame %d (Omicam v%s), selected object: %s", frameCounter, OMICAM_VERSION,
                     fieldObjToString[selectedFieldObject]);
            putText(debugFrame, buf, Point(10, 25), FONT_HERSHEY_DUPLEX, 0.6,
                    Scalar(255, 255, 255), 1, FILLED, false);

            // copy the frame off the Mat into some raw data that can be processed by the remote debugger code
            // frame is a 3 channel RGB image (hence the times 3)
            auto *frameData = (uint8_t*) malloc(frame.rows * frame.cols * 3);
            memcpy(frameData, debugFrame.data, frame.rows * frame.cols * 3);
            
            // wait for localisation to finish before dispatching info
            // if performance issue occur, make this mutexes and shit instead of a busy loop - just couldn't be bothered now
            // FIXME make this not a busy loop and use proper threading stuff
            while (!localiserDone){
                // log_info("Had to wait for localiser before posting to remote debug");
                nanosleep((const struct timespec[]){{0, 1000000L}}, nullptr);
            }

            // ballThresh is just a 1-bit mask so it has only one channel
            auto *threshData = (uint8_t*) calloc(ball.threshMask.rows * ball.threshMask.cols, sizeof(uint8_t));
            switch (selectedFieldObject){
                case OBJ_NONE:
                    // just send the empty buffer
                    remote_debug_post(frameData, threshData, {}, {}, lastFpsMeasurement, debugFrame.cols, debugFrame.rows);
                    break;
                case OBJ_BALL:
                    memcpy(threshData, ball.threshMask.data, ball.threshMask.rows * ball.threshMask.cols);
                    remote_debug_post(frameData, threshData, ball.boundingBox, ball.centroid, lastFpsMeasurement, debugFrame.cols,
                            debugFrame.rows);
                    break;
                case OBJ_LINES:
                    memcpy(threshData, lines.threshMask.data, lines.threshMask.rows * lines.threshMask.cols);
                    remote_debug_post(frameData, threshData, {}, {}, lastFpsMeasurement, debugFrame.cols,
                            debugFrame.rows);
                    break;
                case OBJ_GOAL_BLUE:
                    if (blueGoal.threshMask.data != nullptr)
                        memcpy(threshData, blueGoal.threshMask.data, blueGoal.threshMask.rows * blueGoal.threshMask.cols);
                    remote_debug_post(frameData, threshData, blueGoal.boundingBox, blueGoal.centroid, lastFpsMeasurement,
                            debugFrame.cols, debugFrame.rows);
                    break;
                case OBJ_GOAL_YELLOW:
                    if (yellowGoal.threshMask.data != nullptr)
                        memcpy(threshData, yellowGoal.threshMask.data, yellowGoal.threshMask.rows * yellowGoal.threshMask.cols);
                    remote_debug_post(frameData, threshData, yellowGoal.boundingBox, yellowGoal.centroid, lastFpsMeasurement,
                            debugFrame.cols, debugFrame.rows);
                    break;
            }
        }
#endif

        pthread_testcancel();
    }
    destroyAllWindows();
}

/** monitors the FPS of the vision thread **/
static auto fps_counter_thread(void *arg) -> void *{
    while (true){
        sleep(1);
        double avgTime = movavg_calc(fpsAvg);
        if (avgTime == 0) continue; // another stupid to fix divide by zero when debugging
        lastFpsMeasurement = ROUND2INT(1000.0 / avgTime);
        movavg_clear(fpsAvg);

#if VISION_DIAGNOSTICS
        if (REMOTE_ALWAYS_SEND || !remote_debug_is_connected()){
            printf("Average frame time: %.2f ms, CPU temp: %.2f, FPS: %d\n", avgTime, cpuTemperature, lastFpsMeasurement);
            fflush(stdout);
        }
#endif
    }
}

void vision_init(void){
    cropRect = Rect(visionCropRect[0], visionCropRect[1], visionCropRect[2], visionCropRect[3]);
    fpsAvg = movavg_create(256);

    // IMPORTANT: due to some weird shit with Intel's Turbo Boost on the Celeron, it may be faster to uncomment the below
    // line and disable multi-threading. With multi-threading enabled, all cores max out at 1.4 GHz, whereas with it
    // disabled they will happily run at 2 GHz. However, YMMV so just try it if Omicam is running slow.
    setNumThreads(3); // leave one core free for localiser

    int numCpus = getNumberOfCPUs();
    string features = getCPUFeaturesLine();
    log_info("OpenCV version: %d.%d.%d", CV_VERSION_MAJOR, CV_VERSION_MINOR, CV_VERSION_REVISION);
    log_info("System info: %d CPU(s) available, supported features: %s", numCpus, features.c_str());

    // create threads
    int err = pthread_create(&cvThread, nullptr, cv_thread, nullptr);
    if (err != 0){
        log_error("Failed to create OpenCV thread: %s", strerror(err));
    } else {
        pthread_setname_np(cvThread, "Vision Thread");
    }

    err = pthread_create(&fpsCounterThread, nullptr, fps_counter_thread, nullptr);
    if (err != 0){
        log_error("Failed to create FPS counter thread: %s", strerror(err));
    } else {
        pthread_setname_np(fpsCounterThread, "FPS Counter");
    }

    log_info("Vision started");
    pthread_join(cvThread, nullptr);
    // this will block forever, or until the vision thread terminates for some reason
}

void vision_dispose(void){
    log_trace("Stopping vision thread");
    pthread_cancel(cvThread);
    pthread_cancel(fpsCounterThread);
    pthread_join(cvThread, nullptr);
    pthread_join(fpsCounterThread, nullptr);
    movavg_free(fpsAvg);
}