#include <bits/types/time_t.h>
#include <time.h>
#include "utils.h"
#include <math.h>
#include <stdint.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <log/log.h>

// FIXME deprecate and remove BLOB_USE_NEON since we don't really use it anymore
#if BLOB_USE_NEON
uint8x8_t minBallData, maxBallData, minLineData, maxLineData, minBlueData, maxBlueData, minYellowData, maxYellowData;
#else
int16_t minBallData[3], maxBallData[3], minLineData[3], maxLineData[3], minBlueData[3], maxBlueData[3], minYellowData[3], maxYellowData[3];
#endif

void utils_parse_thresh(char *threshStr, int16_t *array){
    char *token;
    char *threshOrig = strdup(threshStr);
    int16_t i = 0;
    token = strtok(threshStr, ",");

    while (token != NULL){
        char *invalid = NULL;
        int32_t number = strtol(token, &invalid, 10);

        // TODO may want to check the colour range again, it is HSV though which is more difficult
        if (strlen(invalid) != 0){
            log_error("Invalid threshold string \"%s\": invalid token: \"%s\"", threshOrig, invalid);
        } else {
            // put directly into array
            array[i++] = number;

            if (i > 3){
                log_error("Too many values for key: %s (max: 3)", threshOrig);
                return;
            }
        }
        token = strtok(NULL, ",");
    }
    // log_trace("Successfully parsed threshold key: %s", threshOrig);
    free(threshOrig);
}

// source: https://stackoverflow.com/a/3756954/5007892
double utils_get_millis(){
    struct timeval  tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec) * 1000.0 + (tv.tv_usec) / 1000.0;
}