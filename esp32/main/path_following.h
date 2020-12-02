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
#include <DG_dynarr.h>
#include "Vector.h"
#include <math.h>

DA_TYPEDEF(vect_2d_t, point_list_t);

// typedef struct point_list_t {
//     int epic;
// } point_list_t;

// void pf_calculate_nearest(point_list_t points, float robotX, float robotY);
vect_2d_t pf_follow_path(point_list_t points, float robotX, float robotY);

vect_2d_t lowestPoint;
vect_2d_t nextLowest;
float lowestDistance;
vect_2d_t motion; // TODO: Check with Ellis if this is right