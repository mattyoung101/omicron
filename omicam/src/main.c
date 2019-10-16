#define OMX_SKIP64BIT
#include <stdio.h>
#include <stdint.h>
#include "log.h"
#include "omxcam.h"
#include "iniparser.h"
#include "utils.h"
#include "gpu_manager.h"
#include <errno.h>
#include <stdbool.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>

#define OMICAM_VERSION "0.1"

static uint8_t *frameBuffer = NULL; // the actual frame buffer which contains pixels
static uint32_t frameBytesReceived = 0; // bytes received contributing to this frame
static size_t frameBufferSize = 0; // size of the frame buffer in bytes
static uint32_t totalFrames = 0; // total frames processed since last timer call
static uint32_t totalBuffers = 0; // total buffers received since last timer call

static pthread_mutex_t timerMutex;
static pthread_t fpsTimerThread;

/** free resources allocated by the app **/
static void disposeResources(){
    log_trace("Disposing resources");
    free(frameBuffer);
    frameBuffer = NULL;
    gpu_manager_dispose();
    pthread_cancel(fpsTimerThread);
    pthread_mutex_destroy(&timerMutex);
}

/** cleanly shutdown in case of sigint (CTRL C) and sigterm **/
static void signal_handler(int sig){
    log_info("Received signal %s, terminating capture", strsignal(sig));
    omxcam_video_stop();
    disposeResources();
    exit(EXIT_SUCCESS);
}

/** samples and prints FPS every one second - not the best method, but it works **/
static void *fps_timer(void *arg){
    log_trace("FPS timer thread started");

    while (true){
        if (pthread_mutex_lock(&timerMutex)){
            log_debug("FPS: %d (buffers received: %d)", totalFrames, totalBuffers);
            totalFrames = 0;
            totalBuffers = 0;
            pthread_mutex_unlock(&timerMutex);
        }
        sleep(1);
    }
}

/** called when omxcam receives a video buffer **/
static void on_video_data(omxcam_buffer_t buf){
    // naive implementation - problem with this is that if a new frame is in the buffer it's annoying to split it up
    // what we will need to do is use a temporary buffer which we copy the second frame into, process the first frame,
    // then copy the second frame back into the original buffer. so in other words, if we overflow the frame buffer,
    // store the first part of the frame in a tmp buffer, process the first frame, then clear it and copy in the 2nd frame

    // memcpy(frameBuffer + frameBytesReceived, buf.data, buf.length);
    // frameBytesReceived += buf.length;

    for (uint32_t i = 0; i < buf.length; i++){
        frameBuffer[frameBytesReceived++] = buf.data[i];

        if (frameBytesReceived >= frameBufferSize){
            // we must have received a new frame, so process it then clear out the framebuffer and start again
            gpu_manager_post(frameBuffer);

            memset(frameBuffer, 0, frameBufferSize);
            frameBytesReceived = 0;
            PTHREAD_SEM_RUN(&timerMutex, totalFrames++)
        }
        PTHREAD_SEM_RUN(&timerMutex, totalBuffers++)
    }
}

int main() {
    log_set_level(LOG_TRACE);
    log_info("Omicam v%s - Copyright (c) 2019 Team Omicron. All rights reserved.", OMICAM_VERSION);

    log_trace("Loading and parsing config...");
    omxcam_video_settings_t settings;
    settings.on_data = on_video_data;
    settings.format = OMXCAM_FORMAT_RGB888;
    settings.h264.inline_motion_vectors = OMXCAM_TRUE; // unsure if required but used in examples
    // we'll use auto exposure for now but we may want to use sports mode perhaps? OMXCAM_EXPOSURE_SPORTS
    // same thing goes with whitebal, auto should be fine for now
    omxcam_video_init(&settings);

    // apply custom config from ini file
    dictionary *config = iniparser_load("omicam.ini");
    if (config == NULL){
        log_error("Failed to open config file (error: %d)", errno);
        return EXIT_FAILURE;
    }
    iniparser_dump(config, stdout);
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
    log_debug("Allocated %d KB to framebuffer (size: %dx%d)", frameBufferSize / 1024, settings.camera.width,
            settings.camera.height);
    gpu_manager_init(settings.camera.width, settings.camera.height);

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    pthread_mutex_init(&timerMutex, NULL);
    pthread_create(&fpsTimerThread, NULL, fps_timer, NULL);

    log_info("Starting omxcam capture...");
    omxcam_video_start(&settings, OMXCAM_CAPTURE_FOREVER);

    // clean up (shouldn't really reach here, just in case the signal handler doesn't go off)
    disposeResources();
    return EXIT_SUCCESS;
}