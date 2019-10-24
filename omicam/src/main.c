#define OMX_SKIP64BIT
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
#define OMICAM_VERSION "0.1"

static FILE *logFile = NULL;
static pthread_mutex_t logLock;

/** free resources allocated by the app **/
static void disposeResources(){
    log_trace("Disposing resources");
    gpu_manager_dispose();
    camera_manager_dispose();
    remote_debug_dispose();
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
static void log_lock_func(void *userdata, int lock){
    if (lock){
        pthread_mutex_lock(&logLock);
    } else {
        pthread_mutex_unlock(&logLock);
    }
}

int main() {
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
    }
    log_info("Omicam v%s - Copyright (c) 2019 Team Omicron. All rights reserved.", OMICAM_VERSION);

    log_trace("Loading and parsing config...");
    dictionary *config = iniparser_load("../omicam.ini");
    if (config == NULL){
        log_error("Failed to open config file (error: %s)", strerror(errno));
        return EXIT_FAILURE;
    }
    camera_manager_init(config);
    iniparser_freedict(config);

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // this will block the main thread
    camera_manager_capture();
}