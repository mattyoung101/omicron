#pragma once

#include "Vector.h"
#include "stdbool.h"
#include <math.h>

static bool isInEllipse(vect_2d_t robot, float a, float b);
static vect_2d_t ellipsePoint(vect_2d_t robot, float a, float b);
static inline float mCalc(vect_2d_t robot, float a, float b);
static inline float cCalc(vect_2d_t robot, float a, float b);
static inline float x(vect_2d_t robot, float a, float b, int p);
static inline float y(vect_2d_t robot, float a, float b, int p);
static vect_2d_t calcAvoid(float a, float b, vect_2d_t obj, vect_2d_t robot);
vect_2d_t avoidMethod(vect_2d_t avoid, float a, float b, vect_2d_t obj, vect_2d_t robot);