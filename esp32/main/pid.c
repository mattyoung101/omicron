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
#include "pid.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include <inttypes.h>

static int64_t lastTime = 0;
static float integral = 0;

float pid_update(pid_config_t *conf, float input, float setpoint, float modulus){
    float derivative = 0.0f;
    float error = setpoint - input;

    int64_t currentTime = esp_timer_get_time();
    float elapsedTime = (float) (currentTime - lastTime) / 1000000.0f;
    lastTime = currentTime;

    integral += elapsedTime * error;

    if (modulus != 0.0f){
        float difference = (input - conf->lastInput);
        if (difference < -modulus) {
            difference += modulus;
        } else if (difference > modulus) {
            difference -= modulus;
        }

        derivative = difference / elapsedTime;
    } else {
        derivative = (input - conf->lastInput) / elapsedTime;
    }

    conf->lastInput = input;

    float correction = (conf->kp * error) + (conf->ki * integral) - (conf->kd * derivative);
    return conf->absMax == 0 ? correction : constrain(correction, -conf->absMax, conf->absMax);
}