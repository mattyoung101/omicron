#pragma once
#include <math.h>

typedef struct vect_2d_t
{
    float x, y, mag, arg;
} vect_2d_t;

static float calcMag(float x, float y);
static float calcArg(float x, float y);
static float calcX(float mag, float arg);
static float calcY(float mag, float arg);
vect_2d_t vect_2d(float xOrMag, float yOrArg, bool isPolar);
vect_2d_t add_vect_2d(vect_2d_t one, vect_2d_t two);
vect_2d_t subtract_vect_2d(vect_2d_t one, vect_2d_t two);
vect_2d_t scalar_multiply_vect_2d(vect_2d_t one, float scalar);