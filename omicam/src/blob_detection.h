#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t r, g, b;
} colour_t;

/**
 * Posts a frame to the connected component labeler to be processed.
 * @param frame the RGB888 frame data from the GPU, will be freed once this method is done
 * @param width the width of the image in pixels
 * @param height the height of the image in pixels
 */
void blob_detector_post(uint8_t *frame, uint16_t width, uint16_t height);
void blob_detector_run(uint8_t *img);
