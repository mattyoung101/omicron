#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "bcm_host.h"
#include "interface/vcos/vcos.h"
#include "interface/mmal/mmal.h"
#include "interface/mmal/mmal_buffer.h"
#include "interface/mmal/util/mmal_util.h"
#include "interface/mmal/util/mmal_util_params.h"
#include "interface/mmal/util/mmal_default_components.h"
#include "interface/mmal/util/mmal_connection.h"
#include "defines.h"
#if BLOB_USE_NEON
#include <arm_neon.h>
#endif

/** A colour in RGB colour space **/
typedef struct {
    uint8_t r, g, b;
} rgb_colour_t;

/** A range in an array from a min pointer to a max pointer **/
typedef struct {
    uint32_t min;
    uint32_t max;
} array_range_t;

/** RGB offsets **/
typedef enum {
    R = 0,
    G = 1,
    B = 2,
} rgb_t;

#if BLOB_USE_NEON
extern uint8x8_t minBallData, maxBallData, minLineData, maxLineData, minBlueData, maxBlueData, minYellowData, maxYellowData;
#else
extern uint8_t minBallData[3], maxBallData[3], minLineData[3], maxLineData[3], minBlueData[3], maxBlueData[3], minYellowData[3], maxYellowData[3];
#endif

/**
 * Initialises the blob detector, creating work queues, etc
 */
void blob_detector_init(uint16_t width, uint16_t height);
/**
 * Posts a frame to the connected component labeler to be processed.
 * @param buffer the RGB888 frame data from the GPU, will be copied (and freed once this method is done)
 * @param width the width of the image in pixels
 * @param height the height of the image in pixels
 * @return the thresholded image as a RGB pixel buffer, must be freed
 */
uint8_t *blob_detector_post(MMAL_BUFFER_HEADER_T *buffer, uint16_t width, uint16_t height);
void blob_detector_run(uint8_t *img);
#if BLOB_USE_NEON
/** Parses a string in the format "x,y,z" into three numbers to be stored in the given array (vector mode) **/
void blob_detector_parse_thresh(char *threshStr, uint8x8_t *array);
#else
/** Parses a string in the format "x,y,z" into three numbers to be stored in the given array (scalar mode) **/
void blob_detector_parse_thresh(char *threshStr, uint8_t *array);
#endif
/** Destroys the blob detector and its worker threads **/
void blob_detector_dispose(void);
