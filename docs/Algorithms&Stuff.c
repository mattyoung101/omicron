#include "Algorithms&Stuff.h"

// ================================================== VELOCITY CONTROL MODULE ================================================== //

// It's only 1 function so uhhhhh.......

void velcontrol_updatePID(float direction, float speed){
    // Split velocity vector into components
    float polarAngle = bearingToPolar(direction);
    velcontrol_xSpeed = speed * cosf(DEG_RAD * polarAngle);
    velcontrol_ySpeed = speed * sinf(DEG_RAD * polarAngle);

    // Update velocity PIDs
    float xMov = pid_update(&xSpeedPID, mouse_xSpeed, velcontrol_xSpeed, 0.0f);
    float yMov = pid_update(&ySpeedPID, mouse_ySpeed, velcontrol_ySpeed, 0.0f);

    // Convert back to true bearing polar and send off to motor control module
    outputAngle = polarToBearing(atan2f(yMov, xMov) * RAD_DEG);
    outputSpeed = sqrtf(powf(xMov, 2) + powf(yMov, 2));
}