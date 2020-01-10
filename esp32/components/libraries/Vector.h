#pragma once
#include <math.h>
#include <stdbool.h>
#include <defines.h>

typedef struct vect_2d_t {
    float x, y, mag, arg;
} vect_2d_t;

float calcMag(float x, float y) {
    return fmod(sqrtf(pow(x, 2) + pow(y, 2)), 360);
}

float calcArg(float x, float y) {
    return fmod(450 - atan2(y, x) * 180 / PI, 360);
}

float calcX(float mag, float arg) {
    return mag * (cosf(fmod((450-arg)* PI/180,360)));
}

float calcY(float mag, float arg) {
    return mag * (sinf(fmod((450-arg)* PI/180,360)));
}

vect_2d_t vect_2d(float xOrMag, float yOrArg, bool isPolar) {
    vect_2d_t newVect;
    if(isPolar) {
        newVect.mag = xOrMag;
        newVect.arg = yOrArg;
        newVect.x = calcX(xOrMag, yOrArg);
        newVect.y = calcY(xOrMag, yOrArg);
    } else {
        newVect.x = xOrMag;
        newVect.y = yOrArg;
        newVect.mag = calcMag(xOrMag, yOrArg);
        newVect.arg = calcArg(xOrMag, yOrArg); 
    }
    return newVect;
}

vect_2d_t add_vect_2d(vect_2d_t one, vect_2d_t two) {
    vect_2d_t newVect;
    newVect.x = one.x + two.x;
    newVect.y = one.y + two.y;
    newVect.mag = calcMag(newVect.x, newVect.y);
    newVect.arg = calcArg(newVect.x, newVect.y);
    return newVect;
}

vect_2d_t subtract_vect_2d(vect_2d_t one, vect_2d_t two) {
    vect_2d_t newVect;
    newVect.x = one.x - two.x;
    newVect.y = one.y - two.y;
    newVect.mag = calcMag(newVect.x, newVect.y);
    newVect.arg = calcArg(newVect.x, newVect.y);
    return newVect;
}