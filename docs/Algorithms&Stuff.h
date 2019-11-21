#pragma once
#include "../main/pid.h"

#define DEG_RAD 0.017453292519943295 // multiply to convert degrees to radians
#define RAD_DEG 57.29577951308232 // multiply to convert radians to degrees

// All the stuffs for the ball
typedef struct{
    int x; // X position of the ball relative to the center of the robot in mm
    int y; // Y position of the ball relative to the center of the robot in mm
    int angle; // Angle towards the ball in true bearing
    int range; // Distance to the ball in mm
} ball_data_t;

// All the stuffs for the robot
typedef struct{
    int xPos; // X position of the robot relative to the center of the field in mm
    int yPos; // Y position of the robot relative to the center of the field in mm
    float outDirection; // Intended direction of movement
    float outRotation; // The direction the robot needs to face
    float outSpeed; // The intended speed of the robot
} robot_state_t;

// ================================================== UTILS STUFF ================================================== //

float bearingToPolar(float angle){
    return fmodf(angle + 90, 360);
}

float polarToBearing(float angle){
    return fmodf(angle + 270, 360);
}

// ================================================== VELOCITY CONTROL MODULE ================================================== //

// Defines for PID
#define XSPEED_KP 1
#define XSPEED_KI 1
#define XSPEED_KD 1
#define XSPEED_MAX 100

#define YSPEED_KP 1
#define YSPEED_KI 1
#define YSPEED_KD 1
#define YSPEED_MAX 100

float velcontrol_xSpeed;
float velcontrol_ySpeed;
float velcontrol_direction;
float velcontrol_speed;

float mouse_xSpeed;
float mouse_ySpeed;

float outputAngle;
float outputSpeed;

// Config PIDs
static pid_config_t xSpeedPID = {XSPEED_KP, XSPEED_KI, XSPEED_KD, XSPEED_MAX, 0};
static pid_config_t ySpeedPID = {YSPEED_KP, YSPEED_KI, YSPEED_KD, YSPEED_MAX, 0};

void velcontrol_updatePID(float direction, float speed);