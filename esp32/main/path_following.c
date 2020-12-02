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
#include "path_following.h"
#include <utils.h>

static void pf_calculate_nearest(point_list_t points, float robotX, float robotY){
    size_t listSize = da_count(points);
    float m, c, curDistance;
    lowestDistance = -1;
    lowestPoint = vect_2d(0.0f, 0.0f, true);
    nextLowest = vect_2d(0.0f, 0.0f, true);
    vect_2d_t curClosest = vect_2d(0.0f, 0.0f, true);

    for (int i = 0; i < listSize - 1; i++){
        vect_2d_t point = da_get(points, i);
        vect_2d_t nextPoint = da_get(points, i + 1);

        if((nextPoint.x - point.x) == 0) {
            curClosest.x = point.x;
            curClosest.y = robotY;
        } else {
            m = (nextPoint.y - point.y)/(nextPoint.x - point.x);
            c = point.y - m * point.x;
    
            curClosest.x = ((robotX + m * robotY) - m * c)/(powf(m, 2) + 1);
            curClosest.y = (m * (robotX + m * robotY) + c)/(powf(m, 2) + 1);
        }
    
        if (nextPoint.x > point.x) {
            curClosest.x = constrain(curClosest.x, point.x, nextPoint.x);
        } else {
            curClosest.y = constrain(curClosest.y, nextPoint.x, point.x);
        }
    
        if(nextPoint.y > point.y) {
            curClosest.x = constrain(curClosest.y, point.y, nextPoint.y);
        } else {
            curClosest.x = constrain(curClosest.y, nextPoint.y, point.y);
        }

        curDistance = sqrtf(powf((curClosest.x - robotX), 2) + powf((curClosest.y - robotY), 2));

        if(curDistance <= lowestDistance || lowestDistance == -1) {
            lowestDistance = curDistance;
            lowestPoint.x = curClosest.x;
            lowestPoint.y = curClosest.y;
            nextLowest.x = nextPoint.x;
            nextLowest.y = nextPoint.y;
        }
    }
}

vect_2d_t pf_follow_path(point_list_t points, float robotX, float robotY) {
    pf_calculate_nearest(points, robotX, robotY);
    if(lowestDistance <= LINE_RANGE_MIN) {
        motion = add_vect_2d(scalar_multiply_vect_2d(nextLowest, 1 - (lowestDistance/LINE_RANGE_MIN)), scalar_multiply_vect_2d(lowestPoint, lowestDistance/LINE_RANGE_MIN));
    }
    return motion;
}