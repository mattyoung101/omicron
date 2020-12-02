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
#include <lsavoid.h>

// so im not gonna lie i dont know where this is gonna gonna
// TODO: work out where this goes because it'll be the only LS calculation code on the ESP
void movement_avoid_line(vect_2d_t avoidVect) {
    vect_2d_t newMovement = vect_2d(0.0f, 0.0f, false);

    if (is_angle_between(robotState.outMotion.arg, avoidVect.arg + (avoidVect.mag < 0 ? 90 : -90), avoidVect.arg - (avoidVect.mag < 0 ? 90 : -90))) {
        if (avoidVect.mag != 0) {
            avoidVect = add_vect_2d(avoidVect, vect_2d(0.2, avoidVect.arg, true));

            newMovement = vect_2d(robotState.outMotion.mag * sin(fmod(robotState.outMotion.arg - avoidVect.arg, 360) * PI / 180), 90, true);

            if (newMovement.mag < 0) {
                newMovement = vect_2d(fabs(newMovement.mag), 270, true);
            }

            newMovement = vect_2d(newMovement.mag, fmod(newMovement.arg + avoidVect.arg, 360), true);

            if (avoidVect.mag > 0) {
                newMovement = add_vect_2d(newMovement, vect_2d((1 - avoidVect.mag) * 120, avoidVect.arg, true));
            }
        } else {
            newMovement = robotState.outMotion;
        }
        robotState.outMotion = newMovement;
    } else {
        if (avoidVect.mag > 0) {
            vect_2d_t tempVect;
            tempVect = robotState.outMotion;
            tempVect = add_vect_2d(tempVect, vect_2d((1 - avoidVect.mag) * 120, avoidVect.arg, true));
            robotState.outMotion = tempVect;
        }
    }
}
