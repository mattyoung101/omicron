#ifndef UTILS_H
#define UTILS_H

#include <math.h>
#include <Config.h>
#include <Pinlist.h>

#define TO_RADIANS 0.01745329251994329576923690768489

#define TO_DEGREES 57.295779513082320876798154814105

#define ARRAYLENGTH(array) (sizeof(array) / sizeof(array[0]))

#define ARRAYSHIFTDOWN(a, lower, upper){          \
    if (upper == (sizeof(a)/sizeof(a[0])) - 1){   \
        for (int q = upper - 1; q >= lower; q--){ \
            *(a + q + 1) = *(a + q); }            \
    } else {                                      \
        for (int q = upper; q >= lower; q--){     \
            *(a + q + 1) = *(a + q); }}}

int mod(int x, int m);

double doubleMod(double x, double max);

int sign(int value);

int sign(double value);

double degreesToRadians(double degrees);
double radiansToDegrees(double radians);

double doubleAbs(double value);

bool angleIsInside(double angleBoundCounterClockwise, double angleBoundClockwise, double angleCheck);
double angleBetween(double angleCounterClockwise, double angleClockwise);
double smallestAngleBetween(double angle1, double angle2);
double midAngleBetween(double angleCounterClockwise, double angleClockwise);

float lerp(float fromValue, float toValue, float progress);

float get_battery_voltage();

struct Vector3D {
    double x;
    double y;
    double z;
};

#endif
