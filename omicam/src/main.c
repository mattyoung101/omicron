#define OMX_SKIP64BIT
#include <stdio.h>
#include <stdint.h>
#include "log.h"
#include "omxcam.h"
#include "iniparser.h"
#include "utils.h"
#include <errno.h>
#include <stdbool.h>
#include <pthread.h>

#define OMICAM_VERSION "0.1"

/** called when omxcam receives a video frame **/
static void on_video_data(omxcam_buffer_t buf){

}

int main() {
    log_set_level(LOG_TRACE);
    log_info("Omicam v%s - Copyright (c) 2019 Team Omicron. All rights reserved.", OMICAM_VERSION);

    log_trace("Loading and parsing config...");
    omxcam_video_settings_t settings;
    settings.on_data = on_video_data;
    settings.format = OMXCAM_FORMAT_RGB888;
    settings.h264.inline_motion_vectors = OMXCAM_TRUE; // unsure if required but used in examples

    // init default settings
    omxcam_video_init(&settings);

    // apply custom config from ini file
    dictionary *config = iniparser_load("omicam.ini");
    if (config == NULL){
        log_error("Failed to open config file (error: %d)", errno);
        exit(1);
    }
    iniparser_dump(config, stdout);
    INI_LOAD_INT(framerate)
    INI_LOAD_INT(width)
    INI_LOAD_INT(height)
    INI_LOAD_INT(brightness)
    INI_LOAD_INT(contrast)
    INI_LOAD_INT(sharpness)
    INI_LOAD_INT(saturation)
    iniparser_freedict(config);

    log_trace("Starting omxcam...");
    // start a pthread and do cool stuff here

    return 0;
}