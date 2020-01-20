#define DG_DYNARR_IMPLEMENTATION
#include "DG_dynarr.h"
#undef DG_DYNARR_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#undef STB_IMAGE_WRITE_IMPLEMENTATION

#include <stdio.h>
#include "log/log.h"
#include "iniparser/iniparser.h"
#include <errno.h>
#include <stdbool.h>
#include <signal.h>
#include "defines.h"
#include <math.h>
#include <pthread.h>
#include <pwd.h>
#include "remote_debug.h"
#include "utils.h"
#include "computer_vision.hpp"
#include "localisation.h"
#include "comms_uart.h"

static FILE *logFile = NULL;
static pthread_mutex_t logLock;

/** free resources allocated by the app **/
static void disposeResources(){
    log_trace("Disposing resources");
    vision_dispose();
    remote_debug_dispose();
    localiser_dispose();
    comms_uart_dispose();
    log_trace("Closing log file, goodbye!");
    fflush(logFile);
    fclose(logFile);
    pthread_mutex_destroy(&logLock);
}

/** cleanly shutdown in case of sigint (CTRL C) and sigterm **/
static void signal_handler(int sig){
    log_info("Received signal %s, terminating capture", strsignal(sig));
    disposeResources();
    // FIXME due to threading fuckery this is still required
    exit(EXIT_SUCCESS);
}

/** lock the logger to prevent malformed prints across threads **/
static void log_lock_func(void *userdata, int lock){
    if (lock){
        pthread_mutex_lock(&logLock);
    } else {
        pthread_mutex_unlock(&logLock);
    }
}

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGPIPE, SIG_IGN);

#if VERBOSE_LOGGING
    log_set_level(LOG_TRACE);
    puts("Verbose logging enabled.");
#else
    log_set_level(LOG_INFO);
#endif
    pthread_mutex_init(&logLock, NULL);
    log_set_lock(log_lock_func);

//    pthread_t threads[4] = {0};
//    for (int i = 0; i < 4; i++){
//        pthread_create(&threads[i], NULL, bruh_moment, NULL);
//    }
//    puts("spawned hogs");
//    pthread_join(threads[0], NULL);

    struct passwd *pw = getpwuid(getuid());
    char *homedir = pw->pw_dir;
    // FIXME this will fail if the directory doesn't exist!
    logFile = fopen(strcat(homedir, "/Documents/TeamOmicron/Omicam/omicam.log"), "wb");
    if (logFile != NULL){
        log_set_fp(logFile);
    } else {
        fprintf(stderr, "Failed to open log file: %s\n", strerror(errno));
        fprintf(stderr, "ATTENTION: Due to a current bug, you need to make the file ~/Documents/TeamOmicron/Omicam/omicam.log manually"
                        " for logging to work properly.\n");
    }
    log_info("Omicam v%s - Copyright (c) 2019-2020 Team Omicron.", OMICAM_VERSION);
    log_debug("Last full rebuild: %s %s", __DATE__, __TIME__);

    log_debug("Loading and parsing config...");
    dictionary *config = iniparser_load("../omicam.ini");
    if (config == NULL){
        log_error("Failed to open config file (error: %s)", strerror(errno));
        return EXIT_FAILURE;
    }

    char *minBallStr = (char*) iniparser_getstring(config, "Thresholds:minBall", "0,0,0");
    char *maxBallStr = (char*) iniparser_getstring(config, "Thresholds:maxBall", "0,0,0");
    utils_parse_thresh(minBallStr, minBallData);
    utils_parse_thresh(maxBallStr, maxBallData);
    log_trace("Min ball threshold: (%d,%d,%d), Max ball threshold: (%d,%d,%d)", minBallData[0], minBallData[1], minBallData[2],
            maxBallData[0], maxBallData[1], maxBallData[2]);

    char *minLineStr = (char*) iniparser_getstring(config, "Thresholds:minLine", "0,0,0");
    char *maxLineStr = (char*) iniparser_getstring(config, "Thresholds:maxLine", "0,0,0");
    utils_parse_thresh(minLineStr, minLineData);
    utils_parse_thresh(maxLineStr, maxLineData);

    char *minBlueStr = (char*) iniparser_getstring(config, "Thresholds:minBlue", "0,0,0");
    char *maxBlueStr = (char*) iniparser_getstring(config, "Thresholds:maxBlue", "0,0,0");
    utils_parse_thresh(minBlueStr, minBlueData);
    utils_parse_thresh(maxBlueStr, maxBlueData);

    char *minYellowStr = (char*) iniparser_getstring(config, "Thresholds:minYellow", "0,0,0");
    char *maxYellowStr = (char*) iniparser_getstring(config, "Thresholds:maxYellow", "0,0,0");
    utils_parse_thresh(minYellowStr, minYellowData);
    utils_parse_thresh(maxYellowStr, maxYellowData);

    uint16_t width = iniparser_getint(config, "VideoSettings:width", 1280);
    uint16_t height = iniparser_getint(config, "VideoSettings:height", 720);
    char *fieldFile = (char*) iniparser_getstring(config, "Localiser:fieldFile", "CONFIG_ERROR");
    videoWidth = width;
    videoHeight = height;

    char *rectStr = (char*) iniparser_getstring(config, "Vision:cropRect", "0,0,1280,720");
    log_trace("Vision crop rect: %s", rectStr);
    utils_parse_rect(rectStr, visionCropRect);

#if CRANK_THE_MFIN_HOG
    log_warn("Forcing high-performance CPU governing and disabling thermal throttling service (source: CRANK_THE_MFIN_HOG=1)");
    log_warn("This may cause the SBC to overheat! Changes will persist until the next reboot. Monitor thermals carefully.");
    // execute "sudo cpupower frequency-set -g performance" here
    // also execute "sudo systemctl stop thermald"
    // then if they both return successfully print:
    log_debug("Maximum hog cranking configured successfully");
    // otherwise print:
    // log_warn("Failed to crank the mfin hog: %s. Please make sure you've configured password-free sudo access.");
#endif

    // start OpenCV frame grabbing, which blocks the main thread until it's done
    remote_debug_init(width, height);
    localiser_init(fieldFile);
    comms_uart_init();

    fflush(stdout);
    fflush(logFile);
    vision_init();
    log_debug("Omicam terminating normally");

    // this dictionary may be needed by the vision module to initialise some things, so we free it after
    // the application is done
    iniparser_freedict(config);
    // hopefully by now disposeResources() will have already been called, unless we terminated unexpectedly
    // TODO add a variable and check if we already disposed, and if not, do so here
}