/*
 * This file is part of the Teensy 4 firmware project.
 * Copyright (c) 2019-2020 Team Omicron. All rights reserved.
 *
 * Team Omicron members: Lachlan Ellis, Tynan Jones, Ethan Lo,
 * James Talkington, Matt Young.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef Move_h
#define Move_h

#include "Arduino.h"
#include <Pinlist.h>

class Move
{
public:
    void motorCalc(int angle, int dir, int speed);
    void set();
    void move(int speed, int inOnePin, int inTwoPin, int pwmPin, bool reversed, bool brake);
    void go(bool brake);

    void motorTest(int pwm);

    double flmotor_pwm;
    double frmotor_pwm;
    double blmotor_pwm;
    double brmotor_pwm;
private:
    double values[4];
    double radAngle;
};

#endif
