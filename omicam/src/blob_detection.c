#include "blob_detection.h"
#include <math.h>
#include <arm_neon.h>
#include <stdlib.h>

// Rectangle code based on libGDX's Rectangle.java
// https://github.com/libgdx/libgdx/blob/master/gdx/src/com/badlogic/gdx/math/Rectangle.java

// eventually (if this is too slow, which it probably will be) this will use connected component labelling algorithms

static inline bool in_range(colour_t value, colour_t min, colour_t max){
    bool red = value.r >= min.r && value.r <= max.r;
    bool green = value.g >= min.g && value.g <= max.g;
    bool blue = value.b >= min.b && value.b <= max.b;
    return red && green && blue;
}

static inline bool in_range_neon(uint8x8_t value, uint8x8_t min, uint8x8_t max){
    uint8x8_t inMin = vcge_u8(value, min); // "vector compare greater than or equal to"
    uint8x8_t inMax = vcle_u8(value, max); // "vector comprae less than or equal to"
    return inMin[0] && inMax[0] && inMin[1] && inMax[1] && inMin[2] && inMax[2];
}

void blob_detector_post(uint8_t *frame, uint16_t width, uint16_t height){
    uint8_t *processed = malloc(width * height * 3);

    for (int y = 0; y < height; y++){
        for (int x = 0; x < width; x++){
            uint32_t base = x + width * y;
            colour_t pixel = {frame[base + 0], frame[base + 1], frame[base + 2]};
        }
    }

    free(frame);
    free(processed);
}