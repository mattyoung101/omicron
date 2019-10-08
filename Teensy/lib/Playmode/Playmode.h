#ifndef PLAYMODE_H
#define PLAYMODE_H

#include <Arduino.h>
#include <Config.h>
#include <PID.h>
#include <Timer.h>
#include <Vector.h>

class Playmode{
public:
    void init();

    void calculateOrbitTangent();
    void calculateOrbit();

    void calculateDefence(double heading);
    void centre(double heading);

    void calculateLineAvoidance(double heading);
    void crapLineAvoid(double heading);

    void calculateAcceleration();

    void tick(){
        speed = 0;
        direction = 0;
    }

    void updateGoal(int angle, int distance, bool visible);
    void updateLine(double angle, double size, double heading);
    void updateBall(int angle, int strength, bool exists);

    void debug(bool ball, bool line, bool goal); // TODO

    int getDirection(){
        return direction;
    }
    int getSpeed(){
        return speed;
    }

    int getBallAngle(){
        return ballAngle;
    }
    int getBallDistance(){
        return ballStrength;
    }
    bool getBallExist(){
        return ballExists;
    }

    int lineAvoiding(){
        return isOnLine || lineOver;
    }

    int getGoalAngle(){
        return goalAngle;
    }
    int getGoalDistance(){
        return goalLength;
    }
    bool getGoalVisibility(){
        return goalVisible;
    }
    bool brake;
    bool isYeeting;
    int idleDist;
    bool onField = true;
private:
    int direction;
    int speed;

    int goalAngle;
    int goalLength;
    bool goalVisible;

    int camX;
    int camY;

    double lineAngle;
    double lineSize;
    double firstAngle;
    bool isOnLine;
    bool lineOver;
    int urgency;
    double trueLineAngle = NO_LINE_ANGLE;
    double trueLineSize = -1;

    int ballAngle;
    int ballDistance;
    int ballStrength;
    bool ballExists;
    
    Vector current;

    int LRFx;
    int LRFy;
    int frontLRF;
    int rightLRF;
    int backLRF;
    int leftLRF;

    int robotX;
    int robotY;

    PID sidePID = PID(SIDE_KP, SIDE_KI, SIDE_KD, SIDE_MAX);
    PID forwardPID = PID(FORWARD_KP, FORWARD_KI, FORWARD_KD, FORWARD_MAX);

    PID centreSidePID = PID(SIDE_KP, SIDE_KI, SIDE_KD, CENTRE_SIDE_MAX);
    PID centreForwardPID = PID(FORWARD_KP, FORWARD_KI, FORWARD_KD, CENTRE_FORWARD_MAX);

    // Timer accelTimer = Timer(ACCEL_TIME/100);
};

#endif
