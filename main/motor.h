#pragma once
#include <stdbool.h>
#include <math.h>
#include <stdint.h>
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "defines.h"
#include "utils.h"
#include "esp_err.h"
#include "driver/mcpwm.h"

// Ported from Move.cpp

/** Calculates PWM values for each motor **/
void motor_calc(int16_t direction, int16_t orientation, float speed);