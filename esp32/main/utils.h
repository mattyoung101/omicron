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
#include "driver/adc.h"
#include "defines.h"
#include <stdint.h>
#include <math.h>
#include "nvs_flash.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "pid.h"
#include "states.h"
#include "bno055.h"

// literally the world's stupidest hack to make it compile
#ifdef HANDMADE_MATH_IMPLEMENTATION
    #undef HANDMADE_MATH_IMPLEMENTATION
#endif
#include "HandmadeMath.h"

// PIDs
// Orientation Correction PIDs
extern pid_config_t headingPID;
extern pid_config_t goalPID;
extern pid_config_t idlePID;
extern pid_config_t goaliePID;

// Movement PIDs
extern pid_config_t coordPID;
extern pid_config_t sidePID;
extern pid_config_t forwardPID;

// Giant list of macros

/** clip amt between low and high **/
#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
/** x squared **/
#define sq(x) (x * x)
/** first 8 bits of unsigned 16 bit int **/
#define HIGH_BYTE_16(num) ((uint8_t) ((num >> 8) & 0xFF))
/** second 8 bits of unsigned 16 bit int **/
#define LOW_BYTE_16(num)  ((uint8_t) ((num & 0xFF)))
/** unpack two 8 bit integers into a 16 bit integer **/
#define UNPACK_16(a, b) ((uint16_t) ((a << 8) | b))
/** halt in case of irrecoverable error or for debug **/
#define TASK_HALT do { ESP_LOGW(pcTaskGetTaskName(NULL), "Task halting!"); vTaskSuspend(NULL); } while(0);
/** Automated I2C error checking code **/
#define I2C_ERR_CHECK(err) do { if (err != ESP_OK){ \
        ESP_LOGE(TAG, "I2C failure in %s:%d! Error: %s.", __FUNCTION__, __LINE__, esp_err_to_name(err)); \
        i2c_reset_tx_fifo(I2C_NUM_0); \
        i2c_reset_rx_fifo(I2C_NUM_0); \
        return err; \
    } } while (0);
/** Starts counting on the performance timer. The variable "pfBegin" must be undefined **/
#define PERF_TIMER_START int64_t pfBegin = esp_timer_get_time();
/** Stops the performance timer and logs to UART. **/
#define PERF_TIMER_STOP ESP_LOGD("PerfTimer", "%lld us", (esp_timer_get_time() - pfBegin));
/** Cosine in degrees of x in degrees **/
#define cosfd(x) (cosf(x * DEG_RAD) * RAD_DEG)
/** Sin in degrees of x in degrees **/
#define sinfd(x) (sinf(x * DEG_RAD) * RAD_DEG)
/** Sign function **/
#define sign(x) (copysignf(1.0f, x))
/** Brake the motors in the FSM **/
#define FSM_MOTOR_BRAKE do { \
    robotState.outMotion = vect_2d(0, 0, true); \
    robotState.outShouldBrake = false; \
    return; \
} while (0);
/** Switch to a state in attack FSM **/
#define FSM_CHANGE_STATE(STATE) do { fsm_change_state(fsm, &stateAttack ##STATE); return; } while (0);
/** Switch to a state in defence FSM **/
#define FSM_CHANGE_STATE_DEFENCE(STATE) do { fsm_change_state(fsm, &stateDefence ##STATE); return; } while (0);
/** Switch to a state in general fsm */
#define FSM_CHANGE_STATE_GENERAL(STATE) do { fsm_change_state(fsm, &stateGeneral ##STATE); return; } while (0);
/** Revert state in FSM **/
#define FSM_REVERT do { fsm_revert_state(fsm); return; } while (0);
/** Inverts the state of the FSM and also clears history, for use in Bluetooth **/
#define FSM_INVERT_STATE do { \
    /*fsm_partial_reset(stateMachine);*/ \
    if (robotState.outIsAttack){ \
        fsm_change_state(stateMachine, &stateDefenceDefend); \
    } else { \
        fsm_change_state(stateMachine, &stateAttackPursue); \
    } \
} while (0);
/** printf with a newline automatically attached on the end **/
#define printfln(f_, ...) printf((f_ "\n"), __VA_ARGS__)

// Utils functions

int32_t mod(int32_t x, int32_t m);
float floatMod(float x, float m);
int number_comparator_descending(const void *a, const void *b);
float angleBetween(float angleCounterClockwise, float angleClockwise);
float smallestAngleBetween(float angle1, float angle2);
float midAngleBetween(float angleCounterClockwise, float angleClockwise);

/** maps a value to a range **/
int32_t map(int32_t x, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max);
/** linearly interpolates between two values **/
float lerp(float fromValue, float toValue, float progress);

/** Implements Jenkins' One at a Time Hash to calculate the hash code of strings **/
uint32_t str_hash(char *str);

void i2c_scanner(i2c_port_t port);
/** calculates the CRC8 hash of a buffer, source: https://stackoverflow.com/a/51773839/5007892 */
uint8_t crc8(uint8_t *data, size_t len);

/** Returns true if target is between angle1 and angle2. **/
bool is_angle_between(float target, float angle1, float angle2);
/** Runs IMU correction **/
void imu_correction(robot_state_t *robotState);
/** Runs goal correction **/
void goal_correction(robot_state_t *robotState);
void other_goal_correction(robot_state_t *robotState);

float get_magnitude(int16_t x, int16_t y);
float get_angle(int16_t x, int16_t y);
void move_to_xy(robot_state_t *robotState, int16_t x, int16_t y);
/** Orbits around the ball **/
void orbit(robot_state_t *robotState);
/** Positions to arbitrary point on the field **/
void position(robot_state_t *robotState, float distance, float offset, int16_t goalAngle, int16_t goalLength, bool reversed);
/** Quickly moves to a point really quickly **/
void positionFast(robot_state_t *robotState, float distance, float offset, float goalAngle, int16_t goalLength, bool reversed);
/** Does line avoid calculations **/
void update_line(robot_state_t *robotState);
/** Converts a 2D polar vector to cartesian **/
hmm_vec2 vec2_polar_to_cartesian(hmm_vec2 vec);
/** Converts a 2D cartesian vector to polar **/
hmm_vec2 vec2_cartesian_to_polar(hmm_vec2 vec);
/** Reads a uint8_t from NVS and handles errors gracefully **/
void nvs_get_u8_graceful(char *namespace, char *key, uint8_t *value);

// --- DEBUG FUNCTIONS --- //
/** A bunch of functions which just spit relevant information.
 * Just stops me from having to manually typing out the prints.
 * Also looks neater. I think. **/

/** Display ball data **/
void print_ball_data(robot_state_t *robotState);
/** Display line data **/
void print_line_data(robot_state_t *robotState);
/** Display goal data **/
void print_goal_data();
/** Display position data **/
void print_position_data(robot_state_t *robotState);
/** Display motion data **/
void print_motion_data(robot_state_t *robotState);
/** 
 * Logs a message using ESP_LOGD only if it hasn't already been logged since the last call to log_once_reset()
 * log_once_reset() is automatically called by the FSM when a state change or revert occurs
 **/
#define LOG_ONCE(tag, fmt, ...) do { \
    if (log_once_check(fmt)){ \
        ESP_LOGD(tag, fmt, ##__VA_ARGS__); \
    } \
} while (0);

/** Checks if a message has already been printed since the last call to log_once_reset() **/
bool log_once_check(char *msg);
/** Resets the list of already logged messages **/
void log_once_reset();

// BNO055 HAL
s8 bno055_read(u8 dev_addr, u8 reg_addr, u8 *reg_data, u8 cnt);
s8 bno055_write(u8 dev_addr, u8 reg_addr, u8 *reg_data, u8 cnt);
void bno055_delay_ms(u32 msec);

/** Calculates acceleration **/
hmm_vec2 calc_acceleration(float speed, float direction);
