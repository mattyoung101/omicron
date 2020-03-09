/*
 * Copyright (c) 2019 Team Deus Vult (Ethan Lo, Matt Young, Henry Hulbert, Daniel Aziz, Taehwan Kim). 
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "motor.h"

static float pwmValues[4] = {0};
float flmotor_speed = 0;
float frmotor_speed = 0;
float blmotor_speed = 0;
float brmotor_speed = 0;
static const char *TAG = "Motor";

/*
 * Ok, so here's the deal: we have three ways to do PWM: LEDC, MCPWM and Sigma Delta and the best is one MCPWM
 * since it's specifically designed for this task.
 * HOWEVER: it's designed for H-bridges like the L298 which just have 2 pins, but our motor controllers
 * have 3 pins.
 * So what we do instead is a hybrid approach: use MCPWM on all the PWM pins and just regular old
 * GPIO on the IN1 and IN2 pins
 */

void motor_calc(int16_t direction, int16_t orientation, float speed){
    float radAngle = DEG_RAD * ((float) direction + 180.0f);

    pwmValues[0] = cosf(((MOTOR_FL_ANGLE + 90.0f) * DEG_RAD) - radAngle);
    pwmValues[1] = cosf(((MOTOR_FR_ANGLE + 90.0f) * DEG_RAD) - radAngle);
    pwmValues[2] = cosf(((MOTOR_BL_ANGLE + 90.0f) * DEG_RAD) - radAngle);
    pwmValues[3] = cosf(((MOTOR_BR_ANGLE + 90.0f) * DEG_RAD) - radAngle);

    flmotor_speed = speed * pwmValues[0] + orientation;
    frmotor_speed = speed * pwmValues[1] + orientation;
    blmotor_speed = speed * pwmValues[2] + orientation;
    brmotor_speed = speed * pwmValues[3] + orientation;

    float maxSpeed = fmaxf(
        fmaxf(fabsf(flmotor_speed), fabsf(frmotor_speed)), 
        fmaxf(fabsf(blmotor_speed), fabsf(brmotor_speed))
        );

    flmotor_speed = speed == 0 ? flmotor_speed : (flmotor_speed / maxSpeed) * speed;
    frmotor_speed = speed == 0 ? frmotor_speed : (frmotor_speed / maxSpeed) * speed;
    blmotor_speed = speed == 0 ? blmotor_speed : (blmotor_speed / maxSpeed) * speed;
    brmotor_speed = speed == 0 ? brmotor_speed : (brmotor_speed / maxSpeed) * speed;
}