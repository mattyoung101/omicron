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
#include "defines.h"
#include "utils.h"
#include "remote_debug.h"
// yeah this is bad practice, what are you gonna do?
using namespace cv;
using namespace std;

// This file handles all the OpenCV tasks in C++ code, which is then called by the C code in the rest of the project
// Essentially it's just a wrapper for OpenCV

static pthread_t cvThread = {0};

/** this task runs all the computer vision for Omicam, using OpenCV **/
static void *cv_thread(void *arg){
    Scalar minBallScalar = Scalar(minBallData[0], minBallData[1], minBallData[2]);
    Scalar maxBallScalar = Scalar(maxBallData[0], maxBallData[1], maxBallData[2]);
    uint32_t frames = 0;

#if BUILD_TARGET == BUILD_TARGET_PC
    log_trace("Build target is PC, using test data");
    UMat frame = imread("../omicam_thresh_test.png", IMREAD_COLOR).getUMat(ACCESS_READ);
    if (frame.empty()){
        log_error("Unable to load OpenCV test image");
    }
    namedWindow("Omicam (ESC to exit)", WINDOW_AUTOSIZE);
#else
    log_trace("Build target is Jetson, initialising VideoCapture");
    // TODO ... gstreamer and shit ...
    log_trace("OpenCV capture initialised successfully");
#endif

    while (true){
#if BUILD_TARGET == BUILD_TARGET_JETSON
        // get a frame from the capture stream and call it "frame" to match up with the other stuff
#endif
        UMat ballThresh, frameScaled;
        Mat ballLabels, ballStats, ballCentroids;

        // TODO dispatch frame to localiser here (as it runs in parallel so give it a head start)

        // all the image processing is done in these two functions, threshold then find connected components!
        resize(frame, frameScaled, Size(0, 0), VISION_SCALE_FACTOR, VISION_SCALE_FACTOR, INTER_NEAREST);
        inRange(frame, minBallScalar, maxBallScalar, ballThresh);
        int nLabels = connectedComponentsWithStats(ballThresh, ballLabels, ballStats, ballCentroids);

        // find the biggest blob
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
        auto largestId = sortedLabels[0];
        auto largestCentroid = Point(ballCentroids.at<double>(largestId, 0), ballCentroids.at<double>(largestId, 1));

        // dispatch frames to remote debugger
#if DEBUG_ENABLED
        if (frames++ % DEBUG_FRAME_EVERY == 0 && remote_debug_is_connected()) {
            // frame is a 3 channel BGR image (hence the "* 3") which must be converted to RGB
            auto *frameData = (uint8_t*) malloc(frame.rows * frame.cols * 3);
            UMat frameRGB;
            cvtColor(frame, frameRGB, COLOR_BGR2RGB);
            memcpy(frameData, frameRGB.getMat(ACCESS_READ).data, frame.rows * frame.cols * 3);

            // ballThresh is just a 1-bit mask so it has only one channel
            auto *ballThreshData = (uint8_t*) malloc(ballThresh.rows * ballThresh.cols);
            memcpy(ballThreshData, ballThresh.getMat(ACCESS_READ).data, ballThresh.rows * ballThresh.cols);

            // get coords of ball centroid and ball rect to dispatch
            RDPoint ballCentroid = {largestCentroid.x, largestCentroid.y};
            int rectX = ballStats.at<int>(largestId, CC_STAT_LEFT);
            int rectY = ballStats.at<int>(largestId, CC_STAT_TOP);
            int rectW = ballStats.at<int>(largestId, CC_STAT_WIDTH);
            int rectH = ballStats.at<int>(largestId, CC_STAT_HEIGHT);
            RDRect ballRect = {rectX, rectY, rectW, rectH};

            // post to remote debugger
            remote_debug_post(frameData, ballThreshData, ballRect, ballCentroid);
        }
#endif

        // encode protocol buffer to send to ESP over UART
        BallData data = BallData_init_zero;
        data.ballX = largestCentroid.x;
        data.ballY = largestCentroid.y;
        utils_cv_transmit_data(data);

#if BUILD_TARGET == BUILD_TARGET_PC
        // if the target is PC then render some debug info
        // TODO look into sending this Mat instead of labelling stuff in Omicam (probably easier/more efficient?)
        UMat labelDisplay;
        cvtColor(ballThresh, labelDisplay, COLOR_GRAY2RGB);

        // it appears that label 0 is always the background so we skip it (may want to confirm this)
        for (int label = 1; label < nLabels; label++){
            auto centre = Point(ballCentroids.at<double>(label, 0), ballCentroids.at<double>(label, 1));
            int rectX = ballStats.at<int>(label, CC_STAT_LEFT);
            int rectY = ballStats.at<int>(label, CC_STAT_TOP);
            int rectW = ballStats.at<int>(label, CC_STAT_WIDTH);
            int rectH = ballStats.at<int>(label, CC_STAT_HEIGHT);
            auto rect = Rect(rectX, rectY, rectW, rectH);

            Scalar colour = label == largestId ? Scalar(0, 255, 0) : Scalar(0, 0, 255);
            rectangle(labelDisplay, rect, colour, 4);
            circle(labelDisplay, centre, 8, Scalar(0, 0, 255), FILLED);
        }
        imshow("Omicam (ESC to exit)", labelDisplay);

        // wait unless the escape key is pressed
        if (waitKey(50) == 27){
            log_info("Escape key pressed, quitting program");
            break;
        }
#endif
    }

    destroyAllWindows();
#if BUILD_TARGET == BUILD_TARGET_JETSON
    // TODO close video capture
#endif
    return nullptr;
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
    // cout << getBuildInformation();

    log_info("Vision started");
    pthread_join(cvThread, nullptr);
    // this will block forever, or until the vision thread terminates for some reason
}

void vision_dispose(void){
    log_trace("Stopping vision thread");
    pthread_cancel(cvThread);
}