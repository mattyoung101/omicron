#define DG_DYNARR_IMPLEMENTATION
#include "DG_dynarr.h"
#include <stdio.h>
#include "log/log.h"
#include "iniparser/iniparser.h"
#include "gpu_manager.h"
#include <errno.h>
#include <stdbool.h>
#include <signal.h>
#include "defines.h"
#include <math.h>
#include <pthread.h>
#include "camera_manager.h"
#include "remote_debug.h"
#include "utils.h"
#include "blob_detection.h"

#define OMICAM_VERSION "0.1"

static FILE *logFile = NULL;
static pthread_mutex_t logLock;

/** free resources allocated by the app **/
static void disposeResources(){
    log_trace("Disposing resources");
    camera_manager_dispose();
//    gpu_manager_dispose();
    remote_debug_dispose();
    blob_detector_dispose();
    log_trace("Closing log file, goodbye!");
    fclose(logFile);
    pthread_mutex_destroy(&logLock);
}

/** cleanly shutdown in case of sigint (CTRL C) and sigterm **/
static void signal_handler(int sig){
    log_info("Received signal %s, terminating capture", strsignal(sig));
    disposeResources();
    exit(EXIT_SUCCESS);
}

/** lock the logger to prevent malformed prints across threads **/
static void log_lock_func(GCC_UNUSED void *userdata, int lock){
    if (lock){
        pthread_mutex_lock(&logLock);
    } else {
        pthread_mutex_unlock(&logLock);
    }
}

#if BLOB_USE_NEON
#define PRE & // the NEON function takes a pointer to an uint8x8_t
#else
#define PRE // the scalar function just takes an array via a pointer
#endif

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
    logFile = fopen("/home/pi/omicam.log", "w");
    if (logFile != NULL){
        log_set_fp(logFile);
    } else {
        fprintf(stderr, "Failed to open log file: %s\n", strerror(errno));
    }
    log_info("Omicam v%s - Copyright (c) 2019 Team Omicron. All rights reserved.", OMICAM_VERSION);
    log_debug("Build date: %s %s (%d)", __DATE__, __TIME__);

    log_debug("Loading and parsing config...");
    dictionary *config = iniparser_load("../omicam.ini");
    if (config == NULL){
        log_error("Failed to open config file (error: %s)", strerror(errno));
        return EXIT_FAILURE;
    }

    char *minBallStr = (char*) iniparser_getstring(config, "Thresholds:minBall", "0,0,0");
    char *maxBallStr = (char*) iniparser_getstring(config, "Thresholds:maxBall", "0,0,0");
    blob_detector_parse_thresh(minBallStr, PRE minBallData);
    blob_detector_parse_thresh(maxBallStr, PRE maxBallData);

    char *minLineStr = (char*) iniparser_getstring(config, "Thresholds:minLine", "0,0,0");
    char *maxLineStr = (char*) iniparser_getstring(config, "Thresholds:maxLine", "0,0,0");
    blob_detector_parse_thresh(minLineStr, PRE minLineData);
    blob_detector_parse_thresh(maxLineStr, PRE maxLineData);

    char *minBlueStr = (char*) iniparser_getstring(config, "Thresholds:minBlue", "0,0,0");
    char *maxBlueStr = (char*) iniparser_getstring(config, "Thresholds:maxBlue", "0,0,0");
    blob_detector_parse_thresh(minBlueStr, PRE minBlueData);
    blob_detector_parse_thresh(maxBlueStr, PRE maxBlueData);

    char *minYellowStr = (char*) iniparser_getstring(config, "Thresholds:minYellow", "0,0,0");
    char *maxYellowStr = (char*) iniparser_getstring(config, "Thresholds:maxYellow", "0,0,0");
    blob_detector_parse_thresh(minYellowStr, PRE minYellowData);
    blob_detector_parse_thresh(maxYellowStr, PRE maxYellowData);

    camera_manager_init(config);
    iniparser_freedict(config);

    // this will block the main thread
    camera_manager_capture();
}