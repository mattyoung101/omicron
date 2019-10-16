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
#include <pthread.h>
#include <signal.h>

#define OMICAM_VERSION "0.1"

static uint8_t *frameBuffer = NULL; // the actual frame buffer which contains pixels
static uint32_t frameBytesReceived = 0; // bytes received contributing to this frame
static size_t frameBufferSize = 0; // size of the frame buffer in bytes

/** free resources allocated by the app **/
static void disposeResources(){
    free(frameBuffer);
    frameBuffer = NULL;
    gpu_manager_shutdown();
}

/** cleanly shutdown in case of sigint (CTRL C) and sigterm **/
static void signal_handler(int sig){
    log_info("Received signal %s, terminating capture", strsignal(sig));
    omxcam_video_stop();
    disposeResources();
    exit(EXIT_SUCCESS);
}

/**
 * Called when a full frame has been loaded into the frame buffer. frameBuffer and bytesReceived will be reset automatically.
 **/
static void frame_received(){
    // essentially pipe it off to the GPU here
}

/** called when omxcam receives a video buffer **/
static void on_video_data(omxcam_buffer_t buf){
    // FIXME: according to the docs, a new frame may be in the middle of the buffer - maybe we have to copy it manually?
    // perhaps it's faster to check if we overflowed the current frame (frameBytesReceived > frameBufferSize) then backtrack
    // however then we need a bin to put the start of the second frame in
    // you know, gcc will probably optimise our custom memcpy on O3 so it doesn't really matter
    // would it be faster to have a temporary buffer that we memcpy into and

    // naive implementation - problem with this is that if a new frame is in the buffer it's annoying to split it up
    // memcpy(frameBuffer + frameBytesReceived, buf.data, buf.length);
    // frameBytesReceived += buf.length;

    // slow but careful implementation - handles two frames in same buffer correctly
    for (uint32_t i = 0; i < buf.length; i++){
        frameBuffer[frameBytesReceived++] = buf.data[i];

        if (frameBytesReceived >= frameBufferSize){
            // we must have received a new frame, so process it then clear out the framebuffer and start again
            frame_received();
            memset(frameBuffer, 0, frameBufferSize);
            frameBytesReceived = 0;
        }
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

    // init default settings
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
    log_debug("Allocated %d KB to framebuffer (size: %dx%d)", frameBufferSize / 1024, settings.camera.width,
            settings.camera.height);
    frameBuffer = calloc(frameBufferSize, sizeof(uint8_t));

    // create a headless OpenGL ES context for GPU processing
    gpu_manager_init(settings.camera.width, settings.camera.height);

    // capture sigint (ctrl c) and sigterm (program is exited by something else) to cleanly shutdown omxcam
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // start the capture
    log_info("Starting omxcam capture...");
    omxcam_video_start(&settings, OMXCAM_CAPTURE_FOREVER);

    // clean up (shouldn't really reach here, just in case the signal handler doesn't go off)
    disposeResources();
    return EXIT_SUCCESS;
}