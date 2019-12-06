#include "computer_vision.h"
#include <opencv2/core/utility.hpp>
#include "log/log.h"
#include "opencv2/video/tracking.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/core/mat.hpp"
#include "defines.h"

// This file handles all the OpenCV tasks in C++ code, which is then called by the C code in the rest of the project
// Essentially it's just a wrapper for OpenCV

// FIXME GET THE FUCKING PIECE OF SHIT KNOWN AS C++ TO FUCKING BE ABLE TO CALL THE FUCKING LOG FUCKING LIBRARY
static _Atomic bool done = false;
static pthread_t cvThread = {0};

static void *cv_thread(void *arg){
#if BUILD_TARGET == BUILD_TARGET_PC
    log_trace("Build target is PC, using test images");
    cv::UMat frame = cv::imread("../omicam_thresh_test.png", cv::IMREAD_COLOR).getUMat(cv::ACCESS_READ);
    if (frame.empty()){
        log_error("Unable to load OpenCV test image");
    }
#else
    log_trace("Build target is Jetson, initialises capture stream");
    // TODO gstreamer and shit
    log_trace("OpenCV capture initialised successfully");
#endif

    while (true){

    }
    return nullptr;
}

void vision_init(void){
    int err = pthread_create(&cvThread, nullptr, cv_thread, nullptr);
    if (err != 0){
        log_error("Failed to create OpenCV thread: %s", strerror(err));
    } else {
        pthread_setname_np(cvThread, "CV Thread");
    }
    pthread_join(cvThread, nullptr);

    // dispose resources
    log_debug("Capture stopped");
}

void vision_dispose(void){
    log_trace("Stopping OpenCV capture...");
    pthread_cancel(cvThread);
}