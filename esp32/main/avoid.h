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
#pragma once

#include "Vector.h"
#include "stdbool.h"
#include <math.h>

// bool isInEllipse(vect_2d_t robot, float a, float b);
// vect_2d_t ellipsePoint(vect_2d_t robot, float a, float b);
// inline float mCalc(vect_2d_t robot, float a, float b);
// inline float cCalc(vect_2d_t robot, float a, float b);
// inline float x(vect_2d_t robot, float a, float b, int p);
// inline float y(vect_2d_t robot, float a, float b, int p);
// vect_2d_t calcAvoid(float a, float b, vect_2d_t obj, vect_2d_t robot);
vect_2d_t avoidMethod(vect_2d_t avoid, float a, float b, vect_2d_t obj, vect_2d_t robot);