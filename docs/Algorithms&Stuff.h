#pragma once
#include "../main/pid.h"

#define DEG_RAD 0.017453292519943295 // multiply to convert degrees to radians
#define RAD_DEG 57.29577951308232 // multiply to convert radians to degrees

// All the stuffs for the ball
typedef struct{
    int x; // X position of the ball relative to the center of the field in mm
    int y; // Y position of the ball relative to the center of the field in mm
    int angle; // Angle towards the ball in true bearing
    int range; // Distance from robot to ball in mm
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

inline float lerp(float fromValue, float toValue, float progress){
    return fromValue + (toValue - fromValue) * progress;
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

#define COORD_KP 1
#define COORD_KI 1
#define COORD_KD 1
#define COORD_MAX 100

// Placeholder mouse sensor outputs
float mouse_xSpeed;
float mouse_ySpeed;

float outputAngle;
float outputSpeed;

// Config PIDs
static pid_config_t xSpeedPID = {XSPEED_KP, XSPEED_KI, XSPEED_KD, XSPEED_MAX, 0};
static pid_config_t ySpeedPID = {YSPEED_KP, YSPEED_KI, YSPEED_KD, YSPEED_MAX, 0};
static pid_config_t coordPID = {COORD_KP, COORD_KI, COORD_KD, COORD_MAX, 0};

void velcontrol_updatePID(float direction, float speed);
void velcontrol_moveToCoord(int targetX, int targetY, int xPos, int yPos);

// ================================================== ACTION CODE MODULE ================================================== //

// Orbit defines
#define ORBITAL_RADIUS 150 // mm I think
#define ORBIT_SPEED_FAST 500 // mm/s i think
#define ORBIT_SPEED_SLOW 50 // mm/s i think

// Defence defines
#define DEFEND_RADIUS 200 // mm
#define DEFEND_WIDTH 2 // Arbitrary width constant
#define DEFEND_HEIGHT 1 // Arbitrary height constant
#define FIELD_LENGTH 1000 // mm TODO: FIX THIS

// Heading PID defines
#define HEADING_KP 1
#define HEADING_KI 1
#define HEADING_KD 1
#define HEADING_MAX 100

// Goal PID defines
#define GOAL_KP 1
#define GOAL_KI 1
#define GOAL_KD 1
#define GOAL_MAX 100

// Aimbot PID defines
#define AIMBOT_KP 1
#define AIMBOT_KI 1
#define AIMBOT_KD 1
#define AIMBOT_MAX 100

// Config PIDs
static pid_config_t headingPID = {HEADING_KP, HEADING_KI, HEADING_KD, HEADING_MAX, 0};
static pid_config_t goalPID = {GOAL_KP, GOAL_KI, GOAL_KD, GOAL_MAX, 0};
static pid_config_t aimbotPID = {AIMBOT_KP, AIMBOT_KI, AIMBOT_KD, AIMBOT_MAX, 0};

void action_headingCorrection(float heading);
void action_goalCorrection(int xPos, int yPos, bool isGoalie);
void action_aimbot(int xPos, int yPos, bool backKick);

void action_calculateOrbit(int ballX, int ballY, int xPos, int yPos, bool reversed);
void action_calculateDefence(int ballX, int ballY, int xPos, int yPos);