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

void vision_init(void){
#if BUILD_TARGET == BUILD_TARGET_PC
    puts("Build target is PC, using test images");
    cv::UMat frame = cv::imread("../omicam_thresh_test.png", cv::IMREAD_COLOR).getUMat(cv::ACCESS_READ);
#else
    log_trace("Build target is Jetson, initialises capture stream");
    // gstreamer and shit
    log_trace("OpenCV capture initialised successfully");
#endif

    // TODO we're probably gonna have to start up a new fucking thread here since this shit seems to fucking segfault
    // with no fucking help from the useless piece of fucking shit known as asan
    while (!done){
        // read in a new camera frame or just use the old one
    }

    // dispose resources
    puts("Capture stopped");
}

void vision_dispose(void){
    puts("Stopping OpenCV capture...");
    done = true;
}