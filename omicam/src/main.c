#define OMX_SKIP64BIT
#include <stdio.h>
#include <stdint.h>
#include "log/log.h"
#include "omxcam/omxcam.h"
#include "iniparser/iniparser.h"
#include "utils.h"
#include "gpu_manager.h"
#include <errno.h>
#include <stdbool.h>
#include <signal.h>
#include "defines.h"
#include <math.h>

#define OMICAM_VERSION "0.1"

static uint8_t *frameBuffer = NULL; // the actual frame buffer which contains pixels
static uint32_t frameBytesReceived = 0; // bytes received contributing to this frame
static size_t frameBufferSize = 0; // size of the frame buffer in bytes
static double lastFrameTime = 0.0; // how many milliseconds ago the last frame was received
static FILE *logFile = NULL;

/** free resources allocated by the app **/
static void disposeResources(){
    log_trace("Disposing resources");
    free(frameBuffer);
    frameBuffer = NULL;
    gpu_manager_dispose();
    log_trace("Goodbye!");
    fclose(logFile);
}

/** cleanly shutdown in case of sigint (CTRL C) and sigterm **/
static void signal_handler(int sig){
    log_info("Received signal %s, terminating capture", strsignal(sig));
    omxcam_video_stop();
    disposeResources();
    exit(EXIT_SUCCESS);
}

/** called when omxcam receives a video buffer **/
static inline void on_data(omxcam_buffer_t buf){
    // naive implementation - problem with this is that if a new frame is in the buffer it's annoying to split it up
    // what we will need to do is use a temporary buffer which we copy the second frame into, process the first frame,
    // then copy the second frame back into the original buffer. so in other words, if we overflow the frame buffer,
    // store the first part of the frame in a tmp buffer, process the first frame, then clear it and copy in the 2nd frame

     memcpy(frameBuffer + frameBytesReceived, buf.data, buf.length);
     frameBytesReceived += buf.length;

     if (frameBytesReceived >= frameBufferSize){
         double currentTime = utils_get_millis();
         double diff = fabs(currentTime - lastFrameTime);
         printf("Last frame was: %.2f ms ago (%.2f fps)\n", diff, 1000.0 / diff);
         lastFrameTime = currentTime;
         gpu_manager_post(frameBuffer);
         frameBytesReceived = 0;
     }

//    for (uint32_t i = 0; i < buf.length; i++){
//        frameBuffer[frameBytesReceived++] = buf.data[i];
//
//        // check if we received a new frame - if so: post it to GPU, then clear buffers and log FPS
//        if (frameBytesReceived >= frameBufferSize){
//            double currentTime = utils_get_millis();
//            double diff = fabs(currentTime - lastFrameTime);
//            log_trace("Last frame was: %.2f ms ago (%.2f fps)", diff, 1000.0 / diff);
//            lastFrameTime = currentTime;
//            gpu_manager_post(frameBuffer);
//            frameBytesReceived = 0;
//        }
//    }
}

int main() {
#ifdef VERBOSE_LOGGING
    log_set_level(LOG_TRACE);
#else
    log_set_level(LOG_INFO);
#endif
    logFile = fopen("/home/pi/omicam.log", "w");
    if (logFile != NULL){
        log_set_fp(logFile);
    }
    log_info("Omicam v%s - Copyright (c) 2019 Team Omicron. All rights reserved.", OMICAM_VERSION);

    log_trace("Loading and parsing config...");
    omxcam_video_settings_t settings;
    omxcam_video_init(&settings);
    settings.on_data = on_data;
    settings.format = OMXCAM_FORMAT_RGB888;
    settings.h264.inline_motion_vectors = OMXCAM_TRUE;
    settings.camera.exposure = OMXCAM_EXPOSURE_SPORTS;
    // we'll use auto exposure for now but we may want to use sports mode perhaps? OMXCAM_EXPOSURE_SPORTS
    // same thing goes with whitebal, auto should be fine for now

    // apply custom config from ini file
    dictionary *config = iniparser_load("../omicam.ini");
    if (config == NULL){
        log_error("Failed to open config file (error: %s)", strerror(errno));
        return EXIT_FAILURE;
    }
    INI_LOAD_INT(framerate)
    INI_LOAD_INT(width)
    INI_LOAD_INT(height)
    INI_LOAD_INT(brightness)
    INI_LOAD_INT(contrast)
    INI_LOAD_INT(sharpness)
    INI_LOAD_INT(saturation)
    INI_LOAD_INT(shutter_speed)
    INI_LOAD_BOOL(frame_stabilisation)
    INI_LOAD_INT(exposure_compensation)
    iniparser_freedict(config);

    // width of frame * height of frame * colour components
    frameBufferSize = settings.camera.width * settings.camera.height * 3;
    frameBuffer = calloc(frameBufferSize, sizeof(uint8_t));
    log_debug("Allocated %d KB to framebuffer (size: %dx%d), bit depth: 3", frameBufferSize / 1024, settings.camera.width,
            settings.camera.height);
    gpu_manager_init(settings.camera.width, settings.camera.height);

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    log_trace("Starting omxcam capture...");
    omxcam_video_start(&settings, OMXCAM_CAPTURE_FOREVER);
    log_warn("Omxcam capture thread terminated unexpectedly!");

    // clean up (shouldn't really reach here, just in case the signal handler doesn't go off)
    disposeResources();
    return EXIT_SUCCESS;
}