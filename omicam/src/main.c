#include <stdio.h>
#include <stdint.h>
#include "log.h"
#include "omxcam.h"
#include "iniparser.h"
#include <errno.h>
#include <stdbool.h>

#define OMICAM_VERSION "0.1"

/** called when omxcam receives a video frame **/
static void on_video_data(uint8_t *buf, uint32_t length){

}

int main() {
    log_set_level(LOG_TRACE);
    log_info("Omicam v%s - Copyright (c) 2019 Team Omicron. All rights reserved.", OMICAM_VERSION);

    log_trace("Loading and parsing config...");
    omxcam_video_settings_t settings;
    omxcam_video_init(&settings);

    dictionary *config = iniparser_load("omicam.ini");
    if (config == NULL){
        log_error("Failed to open config file (error: %d)", errno);
        exit(1);
    }

    // apply custom config from ini file
    if (iniparser_find_entry(config, "width")){
        settings.camera.width = iniparser_getint(config, "width", -1);
    }
    if (iniparser_find_entry(config, "height")){
        settings.camera.height = iniparser_getint(config, "height", -1);
    }

    iniparser_freedict(config);

    return 0;
}