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
        // get a frame from the capture stream and call it "frame"
#endif
        UMat ballThresh;
        Mat ballLabels;
        auto ballStats = Mat(frame.size(), CV_32S);
        auto ballCentroids = Mat(frame.size(), CV_64F);

        // the loop will look like:
        // 1. threshold (may have to convert to HSV first)
        // 2. find blobs
        // 3. copy UMat to Mat (copy back to CPU), pipe this off to remote debug and localisation
        // 4. send protobuf packet to ESP containing largest blob rect for ball

        inRange(frame, minBallScalar, maxBallScalar, ballThresh);
        int nLabels = connectedComponentsWithStats(ballThresh, ballLabels, ballStats, ballCentroids);

        // map between id and size, again we skip id 0 because that is the background
        map<int, int> labelSizes;
        for (int label = 1; label < nLabels; label++){
            int size = ballStats.at<int>(label, CC_STAT_AREA);
            labelSizes.insert({label, size});
        }

        // find the biggest blob
        // it may be better to rewrite this without a map and instead just use the comparator, look into that in future
        vector<pair<int, int>> pairs;
        pairs.reserve(labelSizes.size());
        for (auto & labelSize : labelSizes){
            pairs.emplace_back(labelSize);
        }
        sort(pairs.begin(), pairs.end(), [=](pair<int, int>& a, pair<int, int>& b){
            return a.second < b.second;
        });
        reverse(pairs.begin(), pairs.end());
        auto largestId = pairs[0].first;
        auto largestCentroid = Point(ballCentroids.at<double>(largestId, 0), ballCentroids.at<double>(largestId, 1));

#if DEBUG_ENABLED
        // dispatch thresholded frame and centroid to remote debugger
        Mat ballThreshCopy;
        ballThresh.copyTo(ballThreshCopy);
#endif

#if BUILD_TARGET == BUILD_TARGET_PC
        // if the target is PC then render some debug info
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
        if (waitKey(250) == 27){
            log_info("Escape key pressed, quitting program");
            break;
        }
#endif
    }
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
    log_info("System info: %d CPU core(s), %d CUDA device(s) available, %s", numCpus, numCudaDevices,
             openCLDevice ? "OpenCL available" : "OpenCL unavailable");

    int err = pthread_create(&cvThread, nullptr, cv_thread, nullptr);
    if (err != 0){
        log_error("Failed to create OpenCV thread: %s", strerror(err));
    } else {
        pthread_setname_np(cvThread, "CV Thread");
    }

    log_info("Vision started");
    pthread_join(cvThread, nullptr);
    // this will block forever, or until the vision thread terminates for some reason
}

void vision_dispose(void){
    log_trace("Stopping vision thread");
    pthread_cancel(cvThread);
}