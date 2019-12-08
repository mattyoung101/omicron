#pragma once
#include <GLES2/gl2.h>
#include <bits/types/FILE.h>
#include "defines.h"

// Misc macros and data

extern int16_t minBallData[3], maxBallData[3], minLineData[3], maxLineData[3], minBlueData[3], maxBlueData[3], minYellowData[3], maxYellowData[3];

/** Parses a string in the format "x,y,z" into three numbers to be stored in the given array  **/
void utils_parse_thresh(char *threshStr, int16_t *array);
/** gets the timestamp in milliseconds **/
double utils_get_millis();
/**
 * Opens the INI file, looks for the key and substitutes in the specified value.
 * @param file the already opened INI file
 * @param key the key to look for, must be unique as this function just finds the first occurrence ignoring sections
 * @param value the value to add in
 */
void utils_ini_update_key(FILE *file, char *key, char *value);

#define GCC_UNUSED __attribute__((unused))

/** first 8 bits of unsigned 16 bit int **/
#define HIGH_BYTE_16(num) ((uint8_t) ((num >> 8) & 0xFF))
/** second 8 bits of unsigned 16 bit int **/
#define LOW_BYTE_16(num)  ((uint8_t) ((num & 0xFF)))
/** unpack two 8 bit integers into a 16 bit integer **/
#define UNPACK_16(a, b) ((uint16_t) ((a << 8) | b))

#define SDL_CHECK(func) do { \
    if ((func) != 0){ \
        log_error("SDL function failed in %s:%d: %s", __FILE__, __LINE__, SDL_GetError()); \
    } \
} while (0);
