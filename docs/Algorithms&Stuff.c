#include "Algorithms&Stuff.h"

robot_state_t robotState = {0};
ball_data_t ballData = {0};

// ================================================== VELOCITY CONTROL MODULE ================================================== //

void velcontrol_updatePID(float direction, float speed){
    // Split velocity vector into components
    float polarAngle = bearingToPolar(direction);
    float velcontrol_xSpeed = speed * cosf(DEG_RAD * polarAngle);
    float velcontrol_ySpeed = speed * sinf(DEG_RAD * polarAngle);

    // Update velocity PIDs
    float xMov = pid_update(&xSpeedPID, mouse_xSpeed, velcontrol_xSpeed, 0.0f);
    float yMov = pid_update(&ySpeedPID, mouse_ySpeed, velcontrol_ySpeed, 0.0f);

    // Convert back to true bearing polar and send off to motor control module
    robotState.outDirection = polarToBearing(atan2f(yMov, xMov) * RAD_DEG);
    robotState.outSpeed = sqrtf(powf(xMov, 2) + powf(yMov, 2));
}

void velcontrol_moveToCoord(int targetX, int targetY, int xPos, int yPos){
    // Calculate movement direction and distance to point
    float targetDirection = polarToBearing(RAD_DEG * atan2f(targetY - yPos, targetX - xPos));
    float distanceToPoint = sqrtf(powf(targetX - xPos, 2) + powf(targetY - yPos, 2));

    // Update coordinate PID and 
    float targetSpeed = pid_update(&coordPID, distanceToPoint, 0.0f, 0.0f);

    // Send to velocity control PIDs
    velcontrol_updatePID(targetDirection, targetSpeed);
}

// ================================================== ACTION CODE MODULE ================================================== //

// See Desmos link for orbit visulisation: https://www.desmos.com/calculator/rd4cuoks3b

void action_calculateOrbit(int ballX, int ballY, int xPos, int yPos, bool reversed){
    // Check which side to orbit
    int orbitalRadius = ballX - xPos < 0 ? -ORBITAL_RADIUS : ORBITAL_RADIUS;

    // Calculate angle and distance to ball
    float orbitalAngle = RAD_DEG * atan2f(ballY - yPos, ballX - xPos - ORBITAL_RADIUS);
    float orbitalDistance = sqrtf(powf(ballX - xPos - ORBITAL_RADIUS, 2) + powf(ballY - yPos, 2));

    // Check if inside the orbital (if so then the tangent calculation will die)
    orbitalDistance = orbitalDistance < ORBITAL_RADIUS ? ORBITAL_RADIUS : orbitalDistance;

    // Calculate the tangent to the orbital and apply reversed orbit
    float tangentAngle = reversed ? orbitalAngle - RAD_DEG * asinf(orbitalRadius / orbitalDistance) : orbitalAngle + RAD_DEG * asinf(orbitalRadius / orbitalDistance);

    // Calculate optimal movement speed
    float angleDifference = fabsf(tangentAngle - (float)ballData.angle);
    float targetSpeed;
    if(reversed){
        targetSpeed = lerp((float)ORBIT_SPEED_FAST, (float)ORBIT_SPEED_SLOW, (1.0 - (float)fabsf(angleDifference) / 90.0));
    } else {
        targetSpeed = lerp((float)ORBIT_SPEED_SLOW, (float)ORBIT_SPEED_FAST, (1.0 - (float)fabsf(angleDifference) / 90.0));
    }

    // Send to velocity control module
    velcontrol_updatePID(polarToBearing(tangentAngle), targetSpeed);
}