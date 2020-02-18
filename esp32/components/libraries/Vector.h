#pragma once
#include <math.h>
#include <stdbool.h>

typedef struct vect_2d_t
{
    float x, y, mag, arg;
} vect_2d_t;

// float calcMag(float x, float y);
// float calcArg(float x, float y);
// float calcX(float mag, float arg);
// float calcY(float mag, float arg);
vect_2d_t vect_2d(float xOrMag, float yOrArg, bool isPolar);
vect_2d_t add_vect_2d(vect_2d_t one, vect_2d_t two);
vect_2d_t subtract_vect_2d(vect_2d_t one, vect_2d_t two);
vect_2d_t scalar_multiply_vect_2d(vect_2d_t one, float scalar);