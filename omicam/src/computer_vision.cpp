#include "computer_vision.h"
#include <opencv2/core/utility.hpp>
#include <iostream>
#include "log/log.h"
#include "opencv2/video/tracking.hpp"
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
// yeah this is bad practice, what are you gonna do?
using namespace cv;
using namespace std;

// This file handles all the OpenCV tasks in C++ code, which is then called by the C code in the rest of the project
// Essentially it's just a wrapper for OpenCV

static pthread_t cvThread = {0};
static pthread_t fpsCounterThread = {0};
static _Atomic int32_t fpsCounter = 0;
static _Atomic int32_t lastFpsMeasurement = 0;

/**
 * Uses the specified threshold values to threshold the given frame and detect the specified object on the field.
 * This is in a separate function so
 */
static void process_object_vision(int32_t *min, int32_t *max, UMat frame){
    UMat thresholded;
    Mat labels, stats, centroids;
    Scalar minScalar = Scalar(min[0], min[1], min[2]);
    Scalar maxScalar = Scalar(max[0], max[1], max[2]);

    inRange(frame, minScalar, maxScalar, thresholded);
    int nLabels = connectedComponentsWithStats(thresholded, labels, stats, centroids);
}

/** this task runs all the computer vision for Omicam, using OpenCV */
static auto cv_thread(void *arg) -> void *{
    uint32_t rdFrameCounter = 0;

#if BUILD_TARGET == BUILD_TARGET_PC
    log_trace("Build target is PC, using test data");
    VideoCapture cap("../orange_ball.m4v");
    if (!cap.isOpened()) {
        log_error("Failed to load OpenCV test video");
    }
    double fps = cap.get(CAP_PROP_FPS);
    log_debug("Video file FPS: %f", fps);
#else
    log_trace("Build target is Jetson, initialising VideoCapture");
    // TODO ... gstreamer and shit ...
    log_trace("OpenCV capture initialised successfully");
#endif

    while (true){
        UMat frame, ballThresh, frameScaled, frameRGB;
        Mat ballLabels, ballStats, ballCentroids;
        Scalar minBallScalar = Scalar(minBallData[0], minBallData[1], minBallData[2]);
        Scalar maxBallScalar = Scalar(maxBallData[0], maxBallData[1], maxBallData[2]);

        cap.read(frame);
        if (frame.empty()){
#if BUILD_TARGET == BUILD_TARGET_PC
            cap.set(CAP_PROP_FRAME_COUNT, 0);
            cap.set(CAP_PROP_POS_FRAMES, 0);
            cap.set(CAP_PROP_POS_AVI_RATIO, 0);
#else
            log_warn("Blank frame received!");
#endif
            continue;
        }

        // all the image processing is done here
        cvtColor(frame, frameRGB, COLOR_BGR2RGB);
        resize(frameRGB, frameScaled, Size(0, 0), VISION_SCALE_FACTOR, VISION_SCALE_FACTOR, INTER_NEAREST);
        inRange(frameRGB, minBallScalar, maxBallScalar, ballThresh);
        int nLabels = connectedComponentsWithStats(ballThresh, ballLabels, ballStats, ballCentroids);

        // TODO dispatch frame to localiser here once we threshold the item

        // find the biggest blob, skipping id 0 which is the background
        vector<int> sortedLabels;
        for (int label = 1; label < nLabels; label++){
            sortedLabels.push_back(label);
        }
        sort(sortedLabels.begin(), sortedLabels.end(), [=](const int &a, const int &b){
            int aSize = ballStats.at<int>(a, CC_STAT_AREA);
            int bSize = ballStats.at<int>(b, CC_STAT_AREA);
            return aSize < bSize;
        });
        reverse(sortedLabels.begin(), sortedLabels.end());

        auto blobExists = !sortedLabels.empty();
        auto largestId = !blobExists ? -1 : sortedLabels[0];
        auto largestCentroid = !blobExists ? Point(0, 0) : Point(ballCentroids.at<double>(largestId, 0),
                ballCentroids.at<double>(largestId, 1));

        // dispatch frames to remote debugger
#if DEBUG_ENABLED
        if (rdFrameCounter++ % DEBUG_FRAME_EVERY == 0 && remote_debug_is_connected()) {
            // frame is a 3 channel BGR image (hence the "* 3") which must be converted to RGB
            auto *frameData = (uint8_t*) malloc(frame.rows * frame.cols * 3);

            char buf[128] = {0};
            snprintf(buf, 128, "Frame %d (Omicam v%s), selected object: %s", rdFrameCounter, OMICAM_VERSION,
                     fieldObjToString[selectedFieldObject]);
            putText(frameRGB, buf, Point(10, 25), FONT_HERSHEY_DUPLEX, 0.5,
                    Scalar(255, 0, 0), 1, FILLED, false);
            memcpy(frameData, frameRGB.getMat(ACCESS_READ).data, frame.rows * frame.cols * 3);

            // ballThresh is just a 1-bit mask so it has only one channel
            auto *ballThreshData = (uint8_t*) calloc(ballThresh.rows * ballThresh.cols, sizeof(uint8_t));
            switch (selectedFieldObject){
                case OBJ_NONE: {
                    // just send the empty buffer
                    break;
                }
                case OBJ_BALL: {
                    memcpy(ballThreshData, ballThresh.getMat(ACCESS_READ).data, ballThresh.rows * ballThresh.cols);
                    break;
                }
                default: {
                    log_warn("Unsupported field object selected: %d", selectedFieldObject);
                    break;
                }
            }

            // get coords of ball centroid and ball rect to dispatch
            if (blobExists) {
                RDPoint ballCentroid = {largestCentroid.x, largestCentroid.y};
                int rectX = ballStats.at<int>(largestId, CC_STAT_LEFT);
                int rectY = ballStats.at<int>(largestId, CC_STAT_TOP);
                int rectW = ballStats.at<int>(largestId, CC_STAT_WIDTH);
                int rectH = ballStats.at<int>(largestId, CC_STAT_HEIGHT);
                RDRect ballRect = {rectX, rectY, rectW, rectH};
                remote_debug_post(frameData, ballThreshData, ballRect, ballCentroid, lastFpsMeasurement);
            } else {
                RDRect ballRect = {0, 0, 0, 0};
                RDPoint ballCentroid = {0, 0};
                remote_debug_post(frameData, ballThreshData, ballRect, ballCentroid, lastFpsMeasurement);
            }
        }
#endif
        // encode protocol buffer to send to ESP over UART
        ObjectData data = ObjectData_init_zero;
        data.ballExists = blobExists;
        data.ballX = largestCentroid.x;
        data.ballY = largestCentroid.y;
        utils_cv_transmit_data(data);

        fpsCounter++;

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
        // log_trace("FPS: %d", lastFpsMeasurement);
    }
}

void vision_init(void){
    int numCpus = getNumberOfCPUs();
    bool openCLDevice = false;
    ocl::setUseOpenCL(true);
    ocl::Context ctx = ocl::Context::getDefault();
    if (ctx.ptr())
        openCLDevice = true;
    int numCudaDevices = cuda::getCudaEnabledDeviceCount();
    log_info("OpenCV version: %d.%d", CV_VERSION_MAJOR, CV_VERSION_MINOR);
    log_info("System info: %d CPU core(s), %d CUDA device(s) available, %s", numCpus, numCudaDevices,
            openCLDevice ? "OpenCL available" : "OpenCL unavailable");

    int err = pthread_create(&cvThread, nullptr, cv_thread, nullptr);
    if (err != 0){
        log_error("Failed to create OpenCV thread: %s", strerror(err));
    } else {
        pthread_setname_np(cvThread, "CV Thread");
    }

    err = pthread_create(&fpsCounterThread, nullptr, fps_counter_thread, nullptr);
    if (err != 0){
        log_error("Failed to create FPS counter thread: %s", strerror(err));
    } else {
        pthread_setname_np(fpsCounterThread, "FPS Counter");
    }
    // cout << getBuildInformation();

    log_info("Vision started");
    pthread_join(cvThread, nullptr);
    // this will block forever, or until the vision thread terminates for some reason
}

void vision_dispose(void){
    log_trace("Stopping vision thread");
    pthread_cancel(cvThread);
}