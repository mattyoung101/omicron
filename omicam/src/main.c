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
#include "tinyexpr.h"

static FILE *logFile = NULL;
static pthread_mutex_t logLock;

/** free resources allocated by the app **/
static void disposeResources(){
    log_trace("Disposing resources");
    vision_dispose();
    remote_debug_dispose();
    localiser_dispose();
    comms_uart_dispose();
    te_free(mirrorModelExpr);
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
    srand(time(NULL)); // NOLINT yes I know, and no I don't care

#if VERBOSE_LOGGING
    log_set_level(LOG_TRACE);
    puts("Verbose logging enabled.");
#else
    log_set_level(LOG_INFO);
#endif
    pthread_mutex_init(&logLock, NULL);
    log_set_lock(log_lock_func);

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
        fprintf(stderr, "Continuing without logging to file, but please fix this!!");
    }
    log_info("Omicam v%s - Copyright (c) 2019-2020 Team Omicron.", OMICAM_VERSION);
    log_debug("Last full rebuild: %s %s", __DATE__, __TIME__);

#if LOADING_REPLAY_FILE
    log_info("Note: loading replay data instead of test data from disk!");
#endif

    log_debug("Loading and parsing config...");
#if BUILD_TARGET == BUILD_TARGET_SBC || LOADING_REPLAY_FILE
    dictionary *config = iniparser_load("../omicam.ini");
#else
    dictionary *config = iniparser_load("../omicam_local.ini");
    log_warn("Loading LOCAL Omicam configuration file (for development use)!");
#endif
    if (config == NULL){
        log_error("Failed to open config file (error: %s)", strerror(errno));
        return EXIT_FAILURE;
    }

    char *minBallStr = (char*) iniparser_getstring(config, "Thresholds:minBall", "0,0,0");
    char *maxBallStr = (char*) iniparser_getstring(config, "Thresholds:maxBall", "0,0,0");
    utils_parse_thresh(minBallStr, minBallData);
    utils_parse_thresh(maxBallStr, maxBallData);

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

    int32_t width = iniparser_getint(config, "VideoSettings:width", 1280);
    int32_t height = iniparser_getint(config, "VideoSettings:height", 720);
    videoWidth = width;
    videoHeight = height;
    isDrawMirrorMask = iniparser_getboolean(config, "Vision:drawMirrorMask", true);
    isDrawRobotMask = iniparser_getboolean(config, "Vision:drawRobotMask", true);
    visionDebugRobotMask = iniparser_getboolean(config, "Vision:renderRobotMask", false);
    visionRecordingEnabled = iniparser_getboolean(config, "Vision:videoRecording", false);
    if (LOADING_REPLAY_FILE && visionRecordingEnabled){
        // this would create duplicates and is easy to forget about
        log_warn("Refusing to enable vision recording whilst playing back a vision recording!");
        visionRecordingEnabled = false;
    }

    const char *mirrorModelStr = iniparser_getstring(config, "Vision:mirrorModel", "x");
    log_trace("Mirror model is: %s", mirrorModelStr);
    te_variable vars[] = {{"x", &mirrorModelVariable}};
    int err;
    mirrorModelExpr = te_compile(mirrorModelStr, vars, 1, &err);
    if (mirrorModelExpr != NULL){
        log_trace("Mirror model parsed successfully!");
    } else {
        log_error("Mirror model is invalid, parse error at %d. Cannot continue.", err);
        return 0;
    }

    visionRobotMaskRadius = iniparser_getint(config, "Vision:robotMaskRadius", 64);
    visionMirrorRadius = iniparser_getint(config, "Vision:mirrorRadius", 256);
    char *rectStr = (char*) iniparser_getstring(config, "Vision:cropRect", "0,0,1280,720");
    log_trace("Vision crop rect: %s", rectStr);
    utils_parse_rect(rectStr, visionCropRect);

    char *fieldFile = (char*) iniparser_getstring(config, "Localiser:fieldFile", "../fields/Ints_StandardField.ff");

    // initialise all the things
    remote_debug_init(width, height);
    localiser_init(fieldFile);
    comms_uart_init();

    fflush(stdout);
    fflush(logFile);
    iniparser_freedict(config);

    vision_init(); // this will block until vision errors/terminates normally

    log_debug("Omicam terminating through main loop exit");
    return EXIT_SUCCESS;
}