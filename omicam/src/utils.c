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
#include <errno.h>

int32_t minBallData[3], maxBallData[3], minLineData[3], maxLineData[3], minBlueData[3], maxBlueData[3], minYellowData[3], maxYellowData[3];
int32_t videoWidth, videoHeight, visionCircleRadius;
int32_t visionCropRect[4];
// OBJ_BALL, OBJ_GOAL_YELLOW, OBJ_GOAL_BLUE,OBJ_LINES,
int32_t *thresholds[] = {minBallData, maxBallData, minYellowData, maxYellowData, minBlueData, maxBlueData, minLineData, maxLineData};
char *fieldObjToString[] = {"OBJ_NONE", "OBJ_BALL", "OBJ_GOAL_YELLOW", "OBJ_GOAL_BLUE", "OBJ_LINES"};

// https://stackoverflow.com/a/1726321/5007892
static void remove_spaces(char* s) {
    const char* d = s;
    do {
        while (*d == ' ') {
            ++d;
        }
    } while ((*s++ = *d++));
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
    uint8_t buf[128] = {0};
    pb_ostream_t stream = pb_ostream_from_buffer(buf, 128);
    if (!pb_encode_delimited(&stream, ObjectData_fields, &ballData)){
        log_error("Failed to encode vision protocol buffer message: %s", PB_GET_ERROR(&stream));
        return;
    }
    comms_uart_send(buf, stream.bytes_written);
}

void utils_ini_update_key(FILE *file, char *key, char *value){
    // so... here we are! string processing in C. this is going to be fun.

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

// source: https://stackoverflow.com/a/3756954/5007892 and https://stackoverflow.com/a/17371925/5007892
double utils_time_millis(){
//    struct timeval tv;
//    gettimeofday(&tv, NULL);
//    return (tv.tv_sec) * 1000.0 + (tv.tv_usec) / 1000.0;
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC_RAW, &time);
    return (time.tv_sec) * 1000.0 + (time.tv_nsec / 1000.0) / 1000.0; // convert tv_sec & tv_usec to millisecond
}