#include "blob_detection.h"
#include "defines.h"
#include <math.h>
#include <arm_neon.h>
#include <stdlib.h>
#include <log/log.h>
#include <omp.h>

// Rectangle code based on libGDX's Rectangle.java
// https://github.com/libgdx/libgdx/blob/master/gdx/src/com/badlogic/gdx/math/Rectangle.java
// eventually (if this is too slow, which it probably will be) this will use connected component labelling algorithms

uint8_t minBallData[3], maxBallData[3], minLineData[3], maxLineData[3], minBlueData[3], maxBlueData[3], minYellowData[3], maxYellowData[3];

#if !BLOB_USE_NEON
/**
 * Thresholds a colour, using scalar instructions (no SIMD/NEON)
 * @param value the colour in question, must be a 3 element array
 * @param min the minimum colour, 3 element array
 * @param max the maximum colour, 3 element array
 * @return whether or not colour is in range of min and max
 */
static inline bool in_range(const uint8_t *value, const uint8_t *min, const uint8_t *max){
    bool red = value[R] >= min[R] && value[R] <= max[R];
    bool green = value[G] >= min[G] && value[G] <= max[G];
    bool blue = value[B] >= min[B] && value[B] <= max[B];
    return red && green && blue;
}
#else
/**
 * Thresholds a colour, using vectors instructions (SIMD/NEON)
 * @param value the colour in question, must be a 3 element array
 * @param min the minimum colour, 3 element array
 * @param max the maximum colour, 3 element array
 * @return whether or not colour is in range of min and max
 */
static inline bool in_range(uint8x8_t value, uint8x8_t min, uint8x8_t max){
    uint8x8_t inMin = vcge_u8(value, min); // "vector compare greater than or equal to"
    uint8x8_t inMax = vcle_u8(value, max); // "vector comprae less than or equal to"
    return inMin[0] && inMax[0] && inMin[1] && inMax[1] && inMin[2] && inMax[2];
}
#endif

uint8_t *blob_detector_post(MMAL_BUFFER_HEADER_T *buffer, uint16_t width, uint16_t height){
    // RGB image where R=line, G=goal (yellow or blue), B=ball
    uint8_t *processed = malloc(width * height * 3);
    uint8_t *frame = buffer->data;

    for (int y = 0; y < height; y++){
        for (int x = 0; x < width; x++){
            uint32_t base = x + width * y + height;

#if BLOB_USE_NEON
            uint8_t scalarColour[3] = {frame[base + R], frame[base + G], frame[base + B]};
            uint8x8_t colour = vld1_u8(scalarColour); // "load a single vector from memory"
#else
            uint8_t colour[3] = {frame[base + R], frame[base + G], frame[base + B]};
#endif

            // bool isLine = in_range(colour, minLineData, maxLineData); // R channel
            // bool isGoal = in_range(colour, minBlueData, maxBlueData) || in_range(colour, minYellowData, maxYellowData); // G channel
            bool isBall = in_range(colour, minBallData, maxBallData); // B channel

            processed[base + R] = /*isLine ? 255 : */0;
            processed[base + G] = /*isGoal ? 255 : */0;
            processed[base + B] = isBall ? 255 : 0;
        }
    }

    // "frame" is owned by the GPU, so we don't free it, and "processed" will be freed later
    return processed;
}

void blob_detector_parse_thresh(char *threshStr, uint8_t *array){
    char *token;
    char *threshOrig = strdup(threshStr);
    uint8_t i = 0;
    token = strtok(threshStr, ",");

    while (token != NULL){
        char *invalid = NULL;
        float number = strtof(token, &invalid);

        if (number > 255){
            log_error("Invalid threshold string \"%s\": token %s > 255 (not in RGB colour range)", threshOrig, token);
        } else if (strlen(invalid) != 0){
            log_error("Invalid threshold string \"%s\": invalid token: \"%s\"", threshOrig, invalid);
        } else {
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