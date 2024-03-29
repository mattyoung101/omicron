/*
 * This file is part of the ESP32 firmware project.
 * Copyright (c) 2019-2020 Team Omicron. All rights reserved.
 *
 * Team Omicron members: Lachlan Ellis, Tynan Jones, Ethan Lo,
 * James Talkington, Matt Young.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "avoid.h"
#include <utils.h>

static bool isInEllipse(vect_2d_t robot, float a, float b)
{
    vect_2d_t temp1 = vect_2d(robot.x, robot.y, false);
    float angle = atan2f(robot.y, robot.x);
    vect_2d_t temp2 = vect_2d(a * cos(angle), b * sin(angle), false);
    return temp2.mag > temp1.mag;
}
static vect_2d_t ellipsePoint(vect_2d_t robot, float a, float b)
{
    //replace with better nearest point function when angus is done
    float angle = atan2f(robot.y, robot.x);
    return vect_2d(a * cosf(angle), b * sinf(angle), false);
}

static inline float mCalc(vect_2d_t robot, float a, float b)
{
    return (-powf(b, 2) * robot.x) / (powf(a, 2) * robot.y);
}

static inline float cCalc(vect_2d_t robot, float a, float b)
{
    return (powf(a, 2) * powf(b, 2)) / (powf(a, 2) * robot.y);
}

static inline float x(vect_2d_t robot, float a, float b, int p)
{
    float m = mCalc(robot, a, b);
    float c = cCalc(robot, a, b);
    return ((-powf(a, 2) * m * c) + p * sign(robot.y) * a * b * sqrtf((powf(a, 2) * powf(m, 2)) + powf(b, 2) - powf(c, 2))) / (powf(b, 2) + (powf(a, 2) * powf(m, 2)));
}

static inline float y(vect_2d_t robot, float a, float b, int p)
{
    float m = mCalc(robot, a, b);
    float c = cCalc(robot, a, b);
    return ((powf(b, 2) * c) + p * sign(robot.y) * a * b * m * sqrtf((powf(a, 2) * powf(m, 2)) + powf(b, 2) - powf(c, 2))) / (powf(b, 2) + (powf(a, 2) * powf(m, 2)));
}

static vect_2d_t calcAvoid(float a, float b, vect_2d_t obj, vect_2d_t robot)
{
    vect_2d_t pointOne = vect_2d(x(robot, a, b, -1), y(robot, a, b, -1), false);
    vect_2d_t pointTwo = vect_2d(x(robot, a, b, 1), y(robot, a, b, 1), false);

    if (isInEllipse(robot, a, b))
    {
        return ellipsePoint(robot, a, b);
    }
    else if (is_angle_between(obj.arg, pointTwo.arg, pointOne.arg))
    {
        if (smallestAngleBetween(obj.arg, pointOne.arg) < smallestAngleBetween(pointTwo.arg, obj.arg))
        {
            return pointTwo;
        }
        else
        {
            return pointOne;
        }
    }
    else
    {
        return obj;
    }
}

vect_2d_t avoidMethod(vect_2d_t avoid, float a, float b, vect_2d_t obj, vect_2d_t robot) {
    vect_2d_t movement = calcAvoid(a, b, obj, vect_2d(robot.x - avoid.x, robot.y - avoid.y, false));
    return vect_2d(movement.x + avoid.x, movement.y + avoid.y, false);
}
