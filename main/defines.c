#include "defines.h"
#include "string.h"

static const char *TAG = "Defines";
bool MOTOR_FL_REVERSED = false;
bool MOTOR_FR_REVERSED = false;
bool MOTOR_BL_REVERSED = false;
bool MOTOR_BR_REVERSED = false;
uint8_t ROBOT_MODE = 254;
uint16_t DRIBBLE_BALL_TOO_FAR;
uint16_t ORBIT_DIST;
uint16_t IN_FRONT_MIN_ANGLE;
uint16_t IN_FRONT_MAX_ANGLE;

// --- Camera --- //
int16_t CAM_OFFSET_X;
int16_t CAM_OFFSET_Y;

// --- Goalie --- //
uint8_t DEFEND_DISTANCE;
uint8_t SURGE_DISTANCE;
uint8_t SURGE_STRENGTH;

// --- Orbit --- //
uint8_t BALL_FAR_STRENGTH;
uint8_t BALL_CLOSE_STRENGTH;
uint8_t ORBIT_SPEED_SLOW;
uint8_t ORBIT_SPEED_FAST;

float ORBIT_CONST;

// Code which sets per-robot values, i.e. values that cannot be set at compile time using #defines

void defines_init(uint8_t robotId){
    ESP_LOGI(TAG, "Initialising values as robot ID #%d", robotId);

    if (robotId == 0){
        ROBOT_MODE = MODE_ATTACK;
        CAM_OFFSET_X = 115;
        CAM_OFFSET_Y = 115;
        DEFEND_DISTANCE = 95;
        SURGE_DISTANCE = 105;
        SURGE_STRENGTH = 65;
        BALL_FAR_STRENGTH = 120;
        BALL_CLOSE_STRENGTH = 40;
        ORBIT_SPEED_SLOW = 30;
        ORBIT_SPEED_FAST = 30;
        ORBIT_CONST = 0.6;
        DRIBBLE_BALL_TOO_FAR = 65;
        ORBIT_DIST = 0;
        IN_FRONT_MIN_ANGLE = 15;
        IN_FRONT_MAX_ANGLE = 355;
    } else {
        ROBOT_MODE = MODE_DEFEND;
        CAM_OFFSET_X = 120;
        CAM_OFFSET_Y = 120;
        DEFEND_DISTANCE = 95; // 24
        SURGE_DISTANCE = 105; // 35
        SURGE_STRENGTH = 65; 
        BALL_FAR_STRENGTH = 120;
        BALL_CLOSE_STRENGTH = 40;
        ORBIT_SPEED_SLOW = 30;
        ORBIT_SPEED_FAST = 30;
        ORBIT_CONST = 0.2;
        DRIBBLE_BALL_TOO_FAR = 60; // TODO FIX THESE VALUES FOR PASSIVE BALL STUFF
        ORBIT_DIST = 0;
        IN_FRONT_MIN_ANGLE = 15;
        IN_FRONT_MAX_ANGLE = 355;
    }
}