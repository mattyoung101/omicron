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
        CAM_OFFSET_X = 120;
        CAM_OFFSET_Y = 120;
        DEFEND_DISTANCE = 65;
        SURGE_DISTANCE = 90;
        SURGE_STRENGTH = 40;
        BALL_FAR_STRENGTH = 90;
        BALL_CLOSE_STRENGTH = 50;
        ORBIT_SPEED_SLOW = 25;
        ORBIT_SPEED_FAST = 50;
        ORBIT_CONST = 0.6;
        DRIBBLE_BALL_TOO_FAR = 50;
        ORBIT_DIST = 0;
        IN_FRONT_MIN_ANGLE = 10;
        IN_FRONT_MAX_ANGLE = 350;
    } else {
        ROBOT_MODE = MODE_DEFEND;
        CAM_OFFSET_X = 120;
        CAM_OFFSET_Y = 120;
        DEFEND_DISTANCE = 70; // 24
        SURGE_DISTANCE = 80; // 35
        SURGE_STRENGTH = 60; 
        BALL_FAR_STRENGTH = 90;
        BALL_CLOSE_STRENGTH = 45;
        ORBIT_SPEED_SLOW = 40;
        ORBIT_SPEED_FAST = 60;
        ORBIT_CONST = 0.2;
        DRIBBLE_BALL_TOO_FAR = 40; // TODO FIX THESE VALUES FOR PASSIVE BALL STUFF
        ORBIT_DIST = 0;
        IN_FRONT_MIN_ANGLE = 10;
        IN_FRONT_MAX_ANGLE = 350;
    }
}