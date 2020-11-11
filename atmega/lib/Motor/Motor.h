/*
 * This file is part of the ATMega firmware project.
 * Copyright (c) 2019-2020 Team Omicron. All rights reserved.
 *
 * Team Omicron members: Lachlan Ellis, Tynan Jones, Ethan Lo,
 * James Talkington, Matt Young.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef MOTOR_H
#define MOTOR_H

class Motor{
    public:
        void init();
        void run(int PWM, int inA, int inB, int pwmPin);
        void move(int frPWM, int brPWM, int blPWM, int flPWM);
};

#endif
