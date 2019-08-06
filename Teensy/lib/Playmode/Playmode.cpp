#include <Playmode.h>
#include <Config.h>
#include <Utils.h>
#include <Arduino.h>
#include <Timer.h>
#include <Pinlist.h>

void Playmode::init(){
    // Reset line avoid variables
    lineOver = false;
    isOnLine = false;
}

// void Playmode::calculateOrbitTangent(){
//     // Simple tangent math stuff
//     if(ballAngle == NO_BALL_ANGLE) ballAngle = NO_BALL_ANGLE;
//     else ballAngle = ballAngle > 180 ? ballAngle - 360 : ballAngle;

//     if(ballAngle == NO_BALL_ANGLE){
//         direction = 0;
//         speed = 0;
//     } else if(angleIsInside(-TSOP_INFRONT, TSOP_INFRONT, ballAngle)){
//         direction = ballAngle/2;
//     } else if(ballDistance <= ORBIT_DISTANCE){
//         direction = ballAngle + 90 * sign(ballAngle);
//     } else {
//         direction = ballAngle + degrees(sin(ORBIT_DISTANCE/ballDistance)) * sign(ballAngle);
//     }
//     speed = ORBIT_SPEED_SLOW + (double)(ORBIT_SPEED_FAST - ORBIT_SPEED_SLOW) * (1.0 - (double)abs(ballAngle-direction) / 90.0);
//     direction *= -1; // Because I'm and idiot and make the TSOPS go anticlockwise XD
// }

void Playmode::calculateOrbit() {
    // Orbit based on ball angle and strength

    if(!ballExists){ // Ball is not visible
        // Serial.println("Stopping");
        speed = 0;
        direction = 0;
        return;
    }
    // Serial.println("Orbiting");

    // Change angle to -180 to 180
    int _ballAngle = ballAngle > 180 ? ballAngle - 360 : ballAngle;
    
    // Add on an angle to the ball angle depending on the ball's angle. Exponential function
    double ballAngleDifference = -sign(ballAngle - 180) * fmin(90, 0.1 * pow(MATH_E, 0.1 * (double)smallestAngleBetween(ballAngle, 0)));

    double strengthFactor = constrain(((double)ballStrength - (double)BALL_FAR_STRENGTH) / ((double)BALL_CLOSE_STRENGTH - BALL_FAR_STRENGTH), 0, 1);

    // Multiply the addition by distance. The further the ball, the more the robot moves towards the ball. Also an exponential function
    double distanceMultiplier = constrain(0.05 * strengthFactor * pow(MATH_E, 4.5 * strengthFactor), 0, 1);
    double angleAddition = ballAngleDifference * distanceMultiplier;

    if(abs(_ballAngle) <= BALL_INFRONT_ANGLE){
        // Serial.println("YEET");
        // direction = ballAngle;
        speed = 100;
        isYeeting = true;
    } else {
        direction = mod(ballAngle + angleAddition, 360);
        speed = ORBIT_SPEED_SLOW + (double)(ORBIT_SPEED_FAST - ORBIT_SPEED_SLOW) * (1.0 - (double)abs(angleAddition) / 90.0);
        isYeeting = false;
    }
    // Serial.println(speed);
}

void Playmode::centre(double heading){
    if (goalVisible) { // Can see the goal
        goalAngle = goalAngle < 0 ? goalAngle + 360 : goalAngle; // Make goal angle between -180 and 180
        double goalAngle_ = doubleMod(goalAngle + heading, 360.0);

        double verticalDistance = goalLength * cos(degreesToRadians(goalAngle_));
        double horizontalDistance = goalLength * sin(degreesToRadians(goalAngle_));

        double distanceMovement = -forwardPID.update(verticalDistance, 35);

        double sidewaysMovement = -sidePID.update(horizontalDistance, 0);

        direction = mod(radiansToDegrees(atan2(sidewaysMovement, distanceMovement)) - (heading), 360);
        speed = sqrt(distanceMovement * distanceMovement + sidewaysMovement * sidewaysMovement);

        // Serial.print(direction);
        // Serial.print("\t");
        // Serial.print(speed);
        // Serial.print("\t");
        // Serial.println(distanceMovement);
    } else {
        direction = 0;
        speed = 0;
    }
}

void Playmode::calculateDefence(double heading){
    if (goalVisible) { // If robot can see the goal
        if (ballExists) { // If the robot can see the ball
            if (angleIsInside(270, 90, ballAngle)) { // If ball is in front of the robot
                if (angleIsInside(340, 20, ballAngle) && ballStrength < SURGE_STRENGTH && goalLength < SURGE_DISTANCE){ // If ball is directly in front of the robot and robot is close to the goal
                    calculateOrbit(); // Surge
                } else { // Centre on the ball
                    double distanceMovement = forwardPID.update(goalLength, DEFEND_DISTANCE);
                    double sidewaysMovement = sidePID.update(mod(ballAngle + 180, 360) - 180, 0);

                    direction = mod(radiansToDegrees(atan2(sidewaysMovement, distanceMovement)), 360);
                    speed = sqrt(distanceMovement * distanceMovement + sidewaysMovement * sidewaysMovement);
                }
            } else { // Orbit to get behind ball
                calculateOrbit();
            }
        } else {
            centre(heading); // Centre in front of the goal
        }
    } else {
        calculateOrbit(); // Can't see goal, just attacking :P
    }
}

// Some black magic bullshit I totoally haven't forgotten how it worked
void Playmode::calculateLineAvoidance(double heading){
    // if(isOnLine && firstAngle != NO_LINE_ANGLE){
    //     if(abs(lineAngle-firstAngle)>LS_LINEOVER_BUFFER && abs(lineAngle-firstAngle)<360-LS_LINEOVER_BUFFER) lineOver = true; // Detecting if we have crossed the line
    //     else lineOver = false;
    // }

    isOnLine = lineAngle != NO_LINE_ANGLE;

    if(onField) {
        if(isOnLine) {
            onField = false;
            trueLineAngle = lineAngle;
            trueLineSize = lineSize;
        }
    } else {
        if(trueLineSize == 3) {
            if(isOnLine) {
                trueLineAngle = doubleMod(lineAngle + 180, 360);
                trueLineSize = 2 - lineSize;
            }
        } else {
            if(!isOnLine) {
                if(trueLineSize <= 1) {
                    onField = true;
                    trueLineSize = -1;
                    trueLineAngle = NO_LINE_ANGLE;
                } else {
                    trueLineSize = 3;
                }
            } else {
                if(smallestAngleBetween(lineAngle, trueLineAngle) <= LS_LINEOVER_BUFFER) {
                    trueLineAngle = lineAngle;
                    trueLineSize = lineSize;
                } else {
                    trueLineAngle = doubleMod(lineAngle + 180, 360);
                    trueLineSize = 2 - lineSize;
                }
            }
        }
    }

    if(!onField){
        if(trueLineSize > LINE_BIG_SIZE){
            direction = doubleMod(trueLineAngle + 180 - heading, 360);
            speed = OVER_LINE_SPEED;
        }else if(trueLineSize >= LINE_SMALL_SIZE && ballExists){
            if(smallestAngleBetween(lineAngle, direction) <= 60){
                speed = 0;
                brake = true;
                // Serial.println("stopping");
            }
        }
    }

    //   if(!onField){
    //     if(lineSize > LINE_BIG_SIZE || lineSize == -1){
    //         if(lineOver){
    //             direction = isOnLine ? doubleMod(lineAngle-heading, 360) : doubleMod(firstAngle-heading+180, 360);
    //         }else{
    //             direction = doubleMod(lineAngle-heading+180, 360);
    //         }
    //         speed = OVER_LINE_SPEED;
    //     }else if(lineSize >= LINE_SMALL_SIZE && ballExists){
    //         if(abs(firstAngle+ballAngle) < 90 && abs(firstAngle+ballAngle) > 270){
    //             direction = doubleMod(firstAngle-heading+180, 360);
    //             speed = 0;
    //             brake = true;
    //             // Serial.println("stopping");
    //         }else{
    //             speed = LINE_TRACK_SPEED;
    //         }
    //     }else{
    //         if(isOnLine) speed *= LINE_SPEED_MULTIPLIER;
    //     }
    // }

    // Check if returned back into the field
    if(!isOnLine && !lineOver) firstAngle = NO_LINE_ANGLE;

    if(!lineOver) firstAngle = lineAngle; // If the robot has just touched the line, we will ignore line over

    // Serial.printf("lineAngle: %f, ballRight: %d, ballLeft: %d\n",lineAngle,ballAngle+90,ballAngle-90);
    Serial.printf("lineAngle: %f, lineSize: %f, trueLineAngle: %f, trueLineSize: %f\n",lineAngle,lineSize,trueLineAngle,trueLineSize);
}

void Playmode::updateGoal(int angle, int distance, bool visible){
    DEFENCE ? goalAngle = angle - 180 : goalAngle = angle > 180 ? angle - 360 : angle;
    goalLength = distance;
    goalVisible = visible;
}

void Playmode::updateLine(double angle, double size, double heading){
    size /= 100;
    isOnLine = angle == NO_LINE_ANGLE ? false : true;
    lineAngle = angle == NO_LINE_ANGLE ? angle : doubleMod(angle+heading, 360);
    // if(lineSize != NO_LINE_SIZE) lineSize = lineOver ? 2 - size : size;
    // else lineSize = size;
    lineSize = size;

    // Serial.printf("isOnLine: %d, lineAngle: %d\n", isOnLine, lineAngle);
}

// y = -27.69ln(x) + 164.05
void Playmode::updateBall(int angle, int strength, bool exists){
    ballDistance = -27.69 * log(strength) + 164.05; // TODO: Fix function
    ballStrength = strength;
    ballAngle = angle;
    ballExists = exists;
}