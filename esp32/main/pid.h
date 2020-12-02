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
#include "esp_timer.h"

// Ported from PID.cpp 

#ifndef constrain
    // stupid hack to fix include stuff
    #define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
#endif

typedef struct {
    float kp, ki, kd, absMax, lastInput;
} pid_config_t;

float pid_update(pid_config_t *conf, float input, float setpoint, float modulus);
