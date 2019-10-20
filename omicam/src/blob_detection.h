#pragma once

#include <stdint.h>
#include <stdbool.h>

/** coordinate of a pixel, using int32_t (we use this instead of HMM_Vec2 so no float ops are required) **/
typedef struct {
    int32_t x, y;
} ipoint_t;

/** a rectangle, in integer coordinates for the image. (x,y) is the lower left corner. **/
typedef struct {
    int32_t x, y, width, height;
} rect_t;

/** returns true if the two rectangles overlap **/
bool rect_overlaps(rect_t *a, rect_t *b);
/** merge rectangle B into rectangle A if they overlap (undefined behaviour if they don't) */
void rect_merge(rect_t *a, rect_t *b);

void blob_detector_run(uint8_t *img);
