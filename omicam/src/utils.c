#include <bits/types/time_t.h>
#include <time.h>
#include "utils.h"
#include <math.h>
#include <stdint.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <log/log.h>
#include "pb.h"
#include "nanopb/pb_encode.h"
#include "comms_uart.h"
#include "computer_vision.hpp"
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <nlopt.h>
#include <iniparser/iniparser.h>
#include <ctype.h>

int32_t minBallData[3], maxBallData[3], minLineData[3], maxLineData[3], minBlueData[3], maxBlueData[3], minYellowData[3], maxYellowData[3];
int32_t videoWidth, videoHeight, visionRobotMaskRadius, visionMirrorRadius;
int32_t visionCropRect[4];
bool isDrawRobotMask = true, isDrawMirrorMask = true;
_Atomic double mirrorModelVariable;
te_expr *mirrorModelExpr;
// OBJ_BALL, OBJ_GOAL_YELLOW, OBJ_GOAL_BLUE, OBJ_LINES
int32_t *thresholds[] = {minBallData, maxBallData, minYellowData, maxYellowData, minBlueData, maxBlueData, minLineData, maxLineData};
char *fieldObjToString[] = {"OBJ_NONE", "OBJ_BALL", "OBJ_GOAL_YELLOW", "OBJ_GOAL_BLUE", "OBJ_LINES"};
bool sleeping;
pthread_cond_t sleepCond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t sleepMutex = PTHREAD_MUTEX_INITIALIZER;
bool sendDebugFrames = true;
struct vec2 localisedPosition = {0};
bool visionRecordingEnabled = false, visionPlayback = false, visionDebugRobotMask = false;

// https://stackoverflow.com/a/1726321/5007892
static void remove_spaces(char* s) {
    const char* d = s;
    do {
        while (*d == ' ') {
            ++d;
        }
    } while ((*s++ = *d++));
}

double utils_camera_dewarp(double x) {
    // FIXME this is not thread safe, we'd need to use a mutex
    mirrorModelVariable = x;
    return te_eval(mirrorModelExpr);
}

void utils_parse_thresh(char *threshStr, int32_t *array){
    char *token;
    char *threshOrig = strdup(threshStr);
    int32_t i = 0;
    remove_spaces(threshStr);
    token = strtok(threshStr, ",");

    while (token != NULL){
        char *invalid = NULL;
        int32_t number = strtol(token, &invalid, 10);

        // TODO may want to check the colour range again? depending on which colour space we're using
        if (strlen(invalid) != 0){
            log_error("Invalid threshold string \"%s\": invalid token: \"%s\"", threshOrig, invalid);
        } else {
            array[i++] = number;
            if (i > 3){
                log_error("Too many elements in threshold value: %s (expected: 3)", threshOrig);
                return;
            }
        }
        token = strtok(NULL, ",");
    }
    free(threshOrig);
}

void utils_parse_rect(char *rectStr, int32_t *array){
    char *token;
    char *rectOrig = strdup(rectStr);
    int32_t i = 0;
    remove_spaces(rectStr);
    token = strtok(rectStr, ",");

    while (token != NULL){
        char *invalid = NULL;
        int32_t number = strtol(token, &invalid, 10);

        if (strlen(invalid) != 0){
            log_error("Invalid rectangle string \"%s\": invalid token: \"%s\"", rectOrig, invalid);
        } else {
            array[i++] = number;
            if (i > 4){
                log_error("Too many elements in rectangle value: %s (expected: 4)", rectOrig);
                return;
            }
        }
        token = strtok(NULL, ",");
    }
    free(rectOrig);
}

void utils_cv_transmit_data(ObjectData ballData){
    // does anybody know why I put this here and not in computer_vision.cpp ...?
    uint8_t msgBuf[128] = {0};
    pb_ostream_t stream = pb_ostream_from_buffer(msgBuf, 128);
    if (!pb_encode(&stream, ObjectData_fields, &ballData)){
        log_error("Failed to encode vision protocol buffer message: %s", PB_GET_ERROR(&stream));
        return;
    }
    comms_uart_send(OBJECT_DATA, msgBuf, stream.bytes_written);
}

static void write_thresholds(FILE *fp){
    fprintf(fp, "minBall = %d,%d,%d\n", minBallData[0], minBallData[1], minBallData[2]);
    fprintf(fp, "maxBall = %d,%d,%d\n\n", maxBallData[0], maxBallData[1], maxBallData[2]);

    fprintf(fp, "minYellow = %d,%d,%d\n", minYellowData[0], minYellowData[1], minYellowData[2]);
    fprintf(fp, "maxYellow = %d,%d,%d\n\n", maxYellowData[0], maxYellowData[1], maxYellowData[2]);

    fprintf(fp, "minBlue = %d,%d,%d\n", minBlueData[0], minBlueData[1], minBlueData[2]);
    fprintf(fp, "maxBlue = %d,%d,%d\n\n", maxBlueData[0], maxBlueData[1], maxBlueData[2]);

    fprintf(fp, "minLine = %d,%d,%d\n", minLineData[0], minLineData[1], minLineData[2]);
    fprintf(fp, "maxLine = %d,%d,%d\n", maxLineData[0], maxLineData[1], maxLineData[2]);
}

void utils_write_thresholds_disk(){
#if BUILD_TARGET == BUILD_TARGET_SBC || VISION_LOAD_TEST_VIDEO
    char *oldFile = "../omicam.ini";
#else
    char *oldFile = "../omicam_local.ini";
#endif

    FILE *curConfig = fopen(oldFile, "r");
    if (curConfig == NULL){
        log_error("Failed to open config file: %s", strerror(errno));
        return;
    }
    FILE *newConfig = fopen("../omicam_new.ini", "w+");
    if (newConfig == NULL){
        log_error("Failed to open new config file: %s", strerror(errno));
        return;
    }
    char lineBuf[255];
    uint32_t line = 0;
    bool skipping = false;

    // read current config line by line, writing out the entire threshold section once we reach OMICAM_THRESH_BEGIN
    // and continuing as normal once we reach OMICAM_THRESH_END
    while (fgets(lineBuf, 255, curConfig) != NULL){
        line++;

        if (skipping){
            // if the "OMICAM_THRESH_END" marker is not in the line, continue skipping, otherwise stop
            if (!strstr(lineBuf, "OMICAM_THRESH_END")){
                continue;
            } else {
                log_trace("Stopping skip on line %d", line);
                skipping = false;
            }
        }
        fputs(lineBuf, newConfig);

        if (strstr(lineBuf, "OMICAM_THRESH_BEGIN")){
            log_trace("Found thresh begin region on line %d", line);
            write_thresholds(newConfig);
            skipping = true;
        }
    }
    fclose(curConfig);
    fclose(newConfig);

    // delete current config file
    if (unlink(oldFile) != 0){
        log_error("Failed to remove old config file: %s", strerror(errno));
        return;
    }

    // rename new config file to old config file
    if (rename("../omicam_new.ini", oldFile)){
        log_error("You will have to rename \"omicam_new.ini\" to \"omicam.ini\" manually, error: %s", strerror(errno));
        return;
    }

    log_debug("New config written to disk successfully!");
}

uint8_t *utils_load_bin(char *path, long *size){
    uint8_t *buf = NULL;
    long length;
    FILE *f = fopen(path, "rb");
    if (!f){
        log_error("Failed to open file \"%s\": %s", path, strerror(errno));
        *size = 0;
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    length = ftell(f);
    *size = length;
    fseek(f, 0, SEEK_SET);
    buf = malloc(length);
    fread(buf, 1, length, f);
    fclose(f);
    return buf;
}

uint8_t crc8(const uint8_t *data, size_t len){
    uint8_t crc = 0xff;
    size_t i, j;
    for (i = 0; i < len; i++) {
        crc ^= data[i];
        for (j = 0; j < 8; j++) {
            if ((crc & 0x80) != 0)
                crc = (uint8_t)((crc << 1) ^ 0x31);
            else
                crc <<= 1;
        }
    }
    return crc;
}

void utils_sleep_enter(void){
    pthread_mutex_lock(&sleepMutex);
    sleeping = true;
    pthread_cond_broadcast(&sleepCond);
    pthread_mutex_unlock(&sleepMutex);
    log_debug("Entered sleep mode successfully");
}

void utils_sleep_exit(void){
    if (!sleeping) return;

    pthread_mutex_lock(&sleepMutex);
    sleeping = false;
    pthread_cond_broadcast(&sleepCond);
    pthread_mutex_unlock(&sleepMutex);
    log_debug("Exited sleep mode successfully");
}

void utils_reload_config(void){
    log_debug("Reloading Omicam INI config from disk...");
#if BUILD_TARGET == BUILD_TARGET_SBC || VISION_LOAD_TEST_VIDEO
    dictionary *config = iniparser_load("../omicam.ini");
#else
    dictionary *config = iniparser_load("../omicam_local.ini");
#endif
    if (config == NULL){
        log_error("Failed to open config file (error: %s)", strerror(errno));
        return;
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

    int32_t width = iniparser_getint(config, "VideoSettings:width", 1280);
    int32_t height = iniparser_getint(config, "VideoSettings:height", 720);
    videoWidth = width;
    videoHeight = height;
    isDrawMirrorMask = iniparser_getboolean(config, "Vision:drawMirrorMask", true);
    isDrawRobotMask = iniparser_getboolean(config, "Vision:drawRobotMask", true);
    visionDebugRobotMask = iniparser_getboolean(config, "Vision:renderRobotMask", false);
    visionRecordingEnabled = iniparser_getboolean(config, "Vision:videoRecording", false);
    if (VISION_LOAD_TEST_VIDEO && visionRecordingEnabled){
        // FIXME also check if we are loading a Protobuf replay (not just a test video)
        log_warn("Refusing to enable vision recording whilst playing back test video!");
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
        log_warn("Mirror model is invalid, parse error at %d.", err);
    }

    visionRobotMaskRadius = iniparser_getint(config, "Vision:robotMaskRadius", 64);
    visionMirrorRadius = iniparser_getint(config, "Vision:mirrorRadius", 256);
    char *rectStr = (char*) iniparser_getstring(config, "Vision:cropRect", "0,0,1280,720");
    log_trace("Vision crop rect: %s", rectStr);
    utils_parse_rect(rectStr, visionCropRect);

    // we don't edit this a lot, and reloading it would require editing the localiser which I can't be stuffed to do
    // char *fieldFile = (char*) iniparser_getstring(config, "Localiser:fieldFile", "../fields/Ints_StandardField.ff");
}

// source: https://stackoverflow.com/a/3756954/5007892 and https://stackoverflow.com/a/17371925/5007892
double utils_time_millis(){
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC_RAW, &time);
    return (time.tv_sec) * 1000.0 + (time.tv_nsec / 1000.0) / 1000.0; // convert tv_sec & tv_usec to millisecond
}

double utils_lerp(double fromValue, double toValue, double progress){
    return fromValue + (toValue - fromValue) * progress;
}

bool utils_only_numbers(char *s){
    while (*s){
        if (!isdigit(*s++)) return false;
    }
    return true;
}

// backported function from new NLopt
const char *nlopt_result_to_string(nlopt_result result)
{
    switch(result)
    {
        case NLOPT_FAILURE: return "FAILURE";
        case NLOPT_INVALID_ARGS: return "INVALID_ARGS";
        case NLOPT_OUT_OF_MEMORY: return "OUT_OF_MEMORY";
        case NLOPT_ROUNDOFF_LIMITED: return "ROUNDOFF_LIMITED";
        case NLOPT_FORCED_STOP: return "FORCED_STOP";
        case NLOPT_SUCCESS: return "SUCCESS";
        case NLOPT_STOPVAL_REACHED: return "STOPVAL_REACHED";
        case NLOPT_FTOL_REACHED: return "FTOL_REACHED";
        case NLOPT_XTOL_REACHED: return "XTOL_REACHED";
        case NLOPT_MAXEVAL_REACHED: return "MAXEVAL_REACHED";
        case NLOPT_MAXTIME_REACHED: return "MAXTIME_REACHED";
    }
    return NULL;
}