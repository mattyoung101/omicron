#include "computer_vision.hpp"
#include <opencv2/core/utility.hpp>
#include "log/log.h"
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/core/mat.hpp"
#include <opencv2/core/ocl.hpp>
#include <opencv2/core/cuda.hpp>
#include <map>
#include <unistd.h>
#include "defines.h"
#include "utils.h"
#include "remote_debug.h"
#include "rpa_queue.h"
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
    RDPoint centroid;
    RDRect boundingBox;
    Mat threshMask;
} object_result_t;

static pthread_t cvThread = {0};
static pthread_t fpsCounterThread = {0};
static _Atomic int32_t fpsCounter = 0;
static _Atomic int32_t lastFpsMeasurement = 0;

/**
 * Uses the specified threshold values to threshold the given frame and detect the specified object on the field.
 * This is in a separate function so that we can just invoke it for each object, likes "ball", "lines", etc.
 * @param frame the thresholded object to be processed
 * @param objectId what the object is
 */
static object_result_t process_object(const Mat& thresholded, field_objects_t objectId){
    Mat labels, stats, centroids;

    // run computer vision tasks
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
    auto largestCentroid = !blobExists ? Point(0, 0) : Point(centroids.at<double>(largestId, 0),
                                                             centroids.at<double>(largestId, 1));
    RDRect objRect = {};
    if (blobExists) {
        int rectX = stats.at<int>(largestId, CC_STAT_LEFT);
        int rectY = stats.at<int>(largestId, CC_STAT_TOP);
        int rectW = stats.at<int>(largestId, CC_STAT_WIDTH);
        int rectH = stats.at<int>(largestId, CC_STAT_HEIGHT);
        objRect = {rectX, rectY, rectW, rectH};
    }

    // scale coordinates if cropping is enabled
#if VISION_CROP_ENABLED
    Rect cropRect(VISION_CROP_RECT);
    objRect.x += cropRect.x;
    objRect.y += cropRect.y;
    largestCentroid.x += cropRect.x;
    largestCentroid.y += cropRect.y;
#endif

    // resize coordinates if image was downscaled
    if (objectId == OBJ_GOAL_BLUE || objectId == OBJ_GOAL_YELLOW){

    }

    object_result_t result = {0}; // NOLINT
    result.exists = blobExists;
    result.centroid = {largestCentroid.x, largestCentroid.y};
    result.boundingBox = objRect;
    result.threshMask = thresholded;

    return result;
}

/**
 * Widens a smaller threshold image to the actual size of the remote debug frame
 * @param input the smaller threshold Mat, it will be scaled in place
 */
static void rescale_threshold_image(const Mat& input){
    resize(input, input, Size(videoWidth, videoHeight), 0, 0, INTER_NEAREST);
}

/** this task runs all the computer vision for Omicam, using OpenCV */
static auto cv_thread(void *arg) -> void *{
    uint32_t rdFrameCounter = 0;
    log_trace("Vision thread started");

#if BUILD_TARGET == BUILD_TARGET_PC
    log_trace("Build target is PC, using test data");
    VideoCapture cap("../orange_ball.m4v");
    if (!cap.isOpened()) {
        log_error("Failed to load OpenCV test video");
    }
    double fps = cap.get(CAP_PROP_FPS);
    log_debug("Video file FPS: %f, Capture API: %s", fps, cap.getBackendName().c_str());
#else
    log_trace("Build target is SBC, initialising VideoCapture");
    VidoCapture cap(0);
    // TODO configure framerate and shit here
    if (!cap.isOpened()){
        log_error("Failed to open OpenCV capture device");
    }
    log_trace("OpenCV capture initialised successfully");
#endif

    // this is a stupid way of calculating this
    Mat junk(videoHeight, videoWidth, CV_8UC1);
    resize(junk, junk, Size(), VISION_SCALE_FACTOR, VISION_SCALE_FACTOR);
    log_info("Scaled frame size: %dx%d (scale factor: %.2f)", junk.cols, junk.rows, VISION_SCALE_FACTOR);

    while (true){
        double begin = utils_get_millis();
        Mat frame, frameScaled;

        cap.read(frame);
        if (frame.empty()){
#if BUILD_TARGET == BUILD_TARGET_PC
            cap.set(CAP_PROP_FRAME_COUNT, 0);
            cap.set(CAP_PROP_POS_FRAMES, 0);
            cap.set(CAP_PROP_POS_AVI_RATIO, 0);
#else
            log_warn("Received empty frame from capture!");
#endif
            continue;
        }

#if VISION_CROP_ENABLED
        frame = frame(Rect(VISION_CROP_RECT));
#endif
        resize(frame, frameScaled, Size(), VISION_SCALE_FACTOR, VISION_SCALE_FACTOR, INTER_NEAREST);

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
            inRange(object == OBJ_GOAL_YELLOW || object == OBJ_GOAL_BLUE ? frameScaled : frame, minScalar, maxScalar, thresholded[object]);
        }, 4);

        // process all our field objects
        auto ball = process_object(thresholded[OBJ_BALL], OBJ_BALL);
        auto yellowGoal = process_object(thresholded[OBJ_GOAL_YELLOW], OBJ_GOAL_YELLOW);
        auto blueGoal = process_object(thresholded[OBJ_GOAL_BLUE], OBJ_GOAL_BLUE);
        auto lines = process_object(thresholded[OBJ_LINES], OBJ_LINES);

        // dispatch frames to remote debugger
#if REMOTE_ENABLED
        if (rdFrameCounter++ % REMOTE_FRAME_INTERVAL == 0 && remote_debug_is_connected()) {
            // we optimised out the cvtColor in the main loop so we gotta do it here instead
            // we can remove it by simply swapping the order of the threshold values to save calling BGR2RGB
            Mat frameRGB;
            cvtColor(frame, frameRGB, COLOR_BGR2RGB);

            char buf[128] = {0};
            snprintf(buf, 128, "Frame %d (Omicam v%s), selected object: %s", rdFrameCounter, OMICAM_VERSION,
                     fieldObjToString[selectedFieldObject]);
            putText(frameRGB, buf, Point(10, 25), FONT_HERSHEY_DUPLEX, 0.6,
                    Scalar(255, 255, 255), 1, FILLED, false);

            // copy the frame off the Mat into some raw data that can be processed by the remote debugger code
            // frame is a 3 channel RGB image (hence the times 3)
            auto *frameData = (uint8_t*) malloc(frame.rows * frame.cols * 3);
            memcpy(frameData, frameRGB.data, frame.rows * frame.cols * 3);

            // ballThresh is just a 1-bit mask so it has only one channel
            auto *threshData = (uint8_t*) calloc(ball.threshMask.rows * ball.threshMask.cols, sizeof(uint8_t));
            switch (selectedFieldObject){
                case OBJ_NONE: {
                    // just send the empty buffer
                    remote_debug_post(frameData, threshData, {}, {}, lastFpsMeasurement, frameRGB.cols, frameRGB.rows);
                    break;
                }
                case OBJ_BALL: {
                    memcpy(threshData, ball.threshMask.data, ball.threshMask.rows * ball.threshMask.cols);
                    remote_debug_post(frameData, threshData, ball.boundingBox, ball.centroid, lastFpsMeasurement, frameRGB.cols, frameRGB.rows);
                    break;
                }
                case OBJ_LINES: {
                    memcpy(threshData, lines.threshMask.data, lines.threshMask.rows * lines.threshMask.cols);
                    remote_debug_post(frameData, threshData, {}, {}, lastFpsMeasurement, frameRGB.cols, frameRGB.rows);
                    break;
                }
                case OBJ_GOAL_BLUE: {
                    // TODO rescale frame and send it, ignore for now
                    break;
                }
                case OBJ_GOAL_YELLOW: {
                    break;
                }
            }
        }
#endif
        // encode protocol buffer to send to ESP over UART
        ObjectData data = ObjectData_init_zero;
        data.ballExists = ball.exists;
        data.ballX = ball.centroid.x;
        data.ballY = ball.centroid.y;
        utils_cv_transmit_data(data);

        fpsCounter++;

        double end = utils_get_millis() - begin;
        // printf("last frame took: %.2f ms to process (%.2f fps) \n", end, 1000.0 / end);

#if BUILD_TARGET == BUILD_TARGET_PC
        waitKey(static_cast<int>(1000 / fps));
#endif
    }
    destroyAllWindows();
}

/** monitors the FPS of the vision thread **/
static auto fps_counter_thread(void *arg) -> void *{
    while (true){
        sleep(1);
        lastFpsMeasurement = fpsCounter;
        fpsCounter = 0;

#if VISION_DIAGNOSTICS
        if (REMOTE_ALWAYS_SEND || !remote_debug_is_connected()){
            printf("Vision FPS: %d\n", lastFpsMeasurement);
            fflush(stdout);
        }
#endif
    }
}

void vision_init(void){
    setUseOptimized(true);
    int numCpus = getNumberOfCPUs();
    string features = getCPUFeaturesLine();
    log_info("OpenCV version: %d.%d.%d", CV_VERSION_MAJOR, CV_VERSION_MINOR, CV_VERSION_REVISION);
    log_info("System info: %d CPU core(s), supported features: %s", numCpus, features.c_str());

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
    pthread_join(cvThread, nullptr);
}