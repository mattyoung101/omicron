#include "Algorithms&Stuff.h"

robot_state_t robotState = {0};
ball_data_t ballData = {0};

// ================================================== VELOCITY CONTROL MODULE ================================================== //

// void thisisafunctionthatjustcontainslistsofpointstoparseforthestrategies() {
    //pretext: fuck proper syntax, we out here writing dogshit pseudocode
    // lineSlide = [[0, -91], [61, -91], [61, 85][35, 85]] || [[0, -91], [-61, -91], [-61, 85][-35, 85]]
    // turtle = [[0, 0], [0, 61.5]]
    // zigzag:
    //     for(i = 0; i <= 8; i++) {
    //         epicnumber = rand(2) //range 0-2
    //         if(epicnumber == 0) {
    //             add [-61, i * 10]
    //         } else if (epicnumber == 1) {
    //             add [0, i * 10]
    //         } else {
    //             add [61, i*10]
    //         }
    //     }
    //     number2 = rand(2) //0-2 again
    //     if(epicnumber == 0) {
    //         add [-62, 85] and [-35, 85]
    //     } else if (epicnumber == 1) {
    //         add [0, 61.5]
    //     } else {
    //         add [62, 85] and [35, 85]
    //     }
    // then also compass correct to [0, 61.5]
        //and then like rotation shit or something idk ????????????????????/

    //search:
    //if sign(x) == 1 {
        // searchpoints = [],[]
    // } else {
        // searchpoints = [],[]
    // }
// }

vect_2d_t orbit() {
    tempVect = subtract_vect_2d(robotPos, ballPos);
     if (sign(tempVect.y) == 1) {
         return orbit.avoidMethod(ballPos, ORBIT_RADIUS, ORBIT_RADIUS, Vector(ballPos.x, ballPos.y - ORBIT_RADIUS, false), robotPos);
    } else {
        if(sign(tempVect.x) == 1) {
            return orbit.avoidMethod(Vector(ballPos.x + ORBIT_RADIUS/2, ballPos.y, false), ORBIT_RADIUS/2, ORBIT_RADIUS, ballPos, robotPos);
        } else {
            return orbit.avoidMethod(Vector(ballPos.x - ORBIT_RADIUS/2, ballPos.y, false), ORBIT_RADIUS/2, ORBIT_RADIUS, ballPos, robotPos);
        }
    }
    rs.outMotion = vect_2d(0, 0, true)
}

vect_2d_t backward_orbit() {
    tempVect = subtract_vect_2d(robotPos, ballPos);
     if (sign(tempVect.y) == -1) {
         return orbit.avoidMethod(ballPos, ORBIT_RADIUS, ORBIT_RADIUS, Vector(ballPos.x, ballPos.y + ORBIT_RADIUS, false), robotPos);
    } else {
        if(sign(tempVect.x) == 1) {
            return orbit.avoidMethod(Vector(ballPos.x + ORBIT_RADIUS/2, ballPos.y, false), ORBIT_RADIUS/2, ORBIT_RADIUS, ballPos, robotPos);
        } else {
            return orbit.avoidMethod(Vector(ballPos.x - ORBIT_RADIUS/2, ballPos.y, false), ORBIT_RADIUS/2, ORBIT_RADIUS, ballPos, robotPos);
        }
    }
    return vect_2d(0, 0, true);
}

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

void action_headingCorrection(float heading){
    robotState.outRotation = pid_update(&headingPID, heading, 0.0f, 0.0f);
}

void action_goalCorrection(int xPos, int yPos, bool isGoalie){
    float goalAngle = polarToBearing(RAD_DEG * atan2(yPos + (isGoalie ? FIELD_LENGTH : -FIELD_LENGTH), xPos));
    if(isGoalie){
        robotState.outRotation = pid_update(&goalPID, floatMod(floatMod(goalAngle, 360.0f), 360.0f) - 180.0f, 0.0f, 0.0f);
    } else {
        robotState.outRotation = pid_update(&goalPID, floatMod(floatMod(goalAngle, 360.0f) + 180.0f, 360.0f) - 180.0f, 0.0f, 0.0f);
    }
}

void action_aimbot(int xPos, int yPos, bool backKick){
    float goalAngle = polarToBearing(RAD_DEG * atan2(yPos - FIELD_LENGTH, xPos));
    if(backKick){
        robotState.outRotation = pid_update(&aimbotPID, floatMod(floatMod(goalAngle, 360.0f), 360.0f) - 180.0f, 0.0f, 0.0f);
    } else {
        robotState.outRotation = pid_update(&aimbotPID, floatMod(floatMod(goalAngle, 360.0f) + 180.0f, 360.0f) - 180.0f, 0.0f, 0.0f);
    }
}

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

    // Calculate movement speed
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

// See Desmos link for defence visualisation: https://www.desmos.com/calculator/2j3gnop9ix
void action_calculateDefence(int ballX, int ballY, int xPos, int yPos){
    // Calculate point to move to
    int targetX;
    int targetY;

    // Check if ball is directly in front (this will result in a divide by zero error)
    if(ballX == 0){
        targetX = 0;
        targetY = sqrtf(DEFEND_HEIGHT * powf(DEFEND_RADIUS, 2));
    } else {
        // Don't ask me what this means, cos even though I wrote it, I couldn't possibly tell you
        targetX = sqrtf((DEFEND_WIDTH * DEFEND_HEIGHT * pow(DEFEND_RADIUS, 2)) / DEFEND_HEIGHT + ((DEFEND_WIDTH * pow(ballY + FIELD_LENGTH, 2)) / pow(ballX, 2))) * signf(ballX);
        targetY = fabsf(((ballY + FIELD_LENGTH) / ballX) * targetX); // Wow something REMOTELY READABLE
    }

    // Send to velocity control module
    velcontrol_moveToCoord(targetX, targetY, xPos, yPos);
}