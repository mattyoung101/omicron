#include "avoid.h"
#include <utils.h>

static bool isInEllipse(vect_2d_t robot, float a, float b)
{
    vect_2d_t temp1 = vect_2d(robot.x, robot.y, false);
    float angle = atan2f(robot.y, robot.x);
    vect_2d_t temp2 = vect_2d(a * cos(angle), b * sin(angle), false);
    return temp2.mag > temp1.mag;
}
static vect_2d_t ellipsePoint(vect_2d_t robot, float a, float b)
{
    //replace with better nearest point function when angus is done
    float angle = atan2f(robot.y, robot.x);
    return vect_2d(a * cosf(angle), b * sinf(angle), false);
}

static inline float mCalc(vect_2d_t robot, float a, float b)
{
    return (-powf(b, 2) * robot.x) / (powf(a, 2) * robot.y);
}

static inline float cCalc(vect_2d_t robot, float a, float b)
{
    return (powf(a, 2) * powf(b, 2)) / (powf(a, 2) * robot.y);
}

static inline float x(vect_2d_t robot, float a, float b, int p)
{
    float m = mCalc(robot, a, b);
    float c = cCalc(robot, a, b);
    return ((-powf(a, 2) * m * c) + p * sign(robot.y) * a * b * sqrtf((powf(a, 2) * powf(m, 2)) + powf(b, 2) - powf(c, 2))) / (powf(b, 2) + (powf(a, 2) * powf(m, 2)));
}

static inline float y(vect_2d_t robot, float a, float b, int p)
{
    float m = mCalc(robot, a, b);
    float c = cCalc(robot, a, b);
    return ((powf(b, 2) * c) + p * sign(robot.y) * a * b * m * sqrtf((powf(a, 2) * powf(m, 2)) + powf(b, 2) - powf(c, 2))) / (powf(b, 2) + (powf(a, 2) * powf(m, 2)));
}

static vect_2d_t calcAvoid(float a, float b, vect_2d_t obj, vect_2d_t robot)
{
    vect_2d_t pointOne = vect_2d(x(robot, a, b, -1), y(robot, a, b, -1), false);
    vect_2d_t pointTwo = vect_2d(x(robot, a, b, 1), y(robot, a, b, 1), false);

    if (isInEllipse(robot, a, b))
    {
        return ellipsePoint(robot, a, b);
    }
    else if (is_angle_between(subtract_vect_2d(obj, robot).arg, subtract_vect_2d(pointTwo, robot).arg, subtract_vect_2d(pointOne, robot).arg))
    {
        if(is_angle_between(obj.arg, pointOne.arg, pointTwo.arg)) {
            return obj; 
        }
        if (smallestAngleBetween(obj.arg, pointOne.arg) < smallestAngleBetween(pointTwo.arg, obj.arg))
        {
            return pointTwo;
        }
        else
        {
            return pointOne;
        }
    }
    else
    {
        return obj;
    }
}

vect_2d_t avoidMethod(vect_2d_t avoid, float a, float b, vect_2d_t obj, vect_2d_t robot) {
    vect_2d_t movement = calcAvoid(a, b, vect_2d(obj.x - avoid.x, obj.y - avoid.y, false), vect_2d(robot.x - avoid.x, robot.y - avoid.y, false));
    return vect_2d(movement.x + avoid.x, movement.y + avoid.y, false);
}

vect_2d_t defendAvoid(vect_2d_t ball, float radius, vect_2d_t otherRobot, vect_2d_t obj, vect_2d_t robot) {

    float ballRadius = radius;
    float robotRadius = radius;
    vect_2d_t ballTangents[2] = {vect_2d(x(subtract_vect_2d(robot, ball), ballRadius, ballRadius, -1), y(subtract_vect_2d(robot, ball), ballRadius, ballRadius, -1), false), vect_2d(x(subtract_vect_2d(robot, ball), ballRadius, ballRadius, 1), y(subtract_vect_2d(robot, ball), ballRadius, ballRadius, 1), false)};
    vect_2d_t robotTangents[2] = {vect_2d(x(subtract_vect_2d(robot, otherRobot), robotRadius, robotRadius, -1), y(subtract_vect_2d(robot, otherRobot), robotRadius, robotRadius, -1), false), vect_2d(x(subtract_vect_2d(robot, otherRobot), robotRadius, robotRadius, 1), y(subtract_vect_2d(robot, otherRobot), robotRadius, robotRadius, 1), false)};


    bool pastBall = is_angle_between(subtract_vect_2d(obj, robot).arg, subtract_vect_2d(ballTangents[0], robot).arg, subtract_vect_2d(ballTangents[1], robot).arg);
    bool pastRobot =  is_angle_between(subtract_vect_2d(obj, robot).arg, subtract_vect_2d(robotTangents[0], robot).arg, subtract_vect_2d(robotTangents[1], robot).arg);
    if (pastBall && pastRobot) {
        float angles[6] = {angleBetween(ballTangents[0].arg, ballTangents[1].arg), angleBetween(robotTangents[0].arg, robotTangents[1].arg), angleBetween(robotTangents[0].arg, ballTangents[0].arg), angleBetween(robotTangents[0].arg, ballTangents[1].arg), angleBetween(robotTangents[1].arg, ballTangents[0].arg), angleBetween(robotTangents[1].arg, robotTangents[1].arg)};
        vect_2d_t relPoints[6][2] = {{ballTangents[0], ballTangents[1]}, {robotTangents[0], robotTangents[1]}, {robotTangents[0], ballTangents[0]}, {robotTangents[0], ballTangents[1]}, {robotTangents[1], ballTangents[0]}, {robotTangents[1], ballTangents[1]}};
        float maxi = 0;
        int pos;
        for(int g = 0; g < 6; g++) {
            if(angles[g] > maxi) {
                maxi = angles[g];
                pos = g;
            }
        }

        vect_2d_t tangents[2];
        tangents[0] = relPoints[pos][0];
        tangents[1] = relPoints[pos][1];

        //do some shit here to do the thing i guess
        return vect_2d(0, 0, true);
    } else if(pastBall && !pastRobot) {
        vect_2d_t movement = calcAvoid(radius, radius, vect_2d(obj.x - ball.x, obj.y - ball.y, false), vect_2d(robot.x - ball.x, robot.y - ball.y, false));
        return vect_2d(movement.x + ball.x, movement.y + ball.y, false);
    } else if (!pastBall && pastRobot) {
        vect_2d_t movement = calcAvoid(radius, radius, vect_2d(obj.x - otherRobot.x, obj.y - otherRobot.y, false), vect_2d(robot.x - otherRobot.x, robot.y - otherRobot.y, false));
        return vect_2d(movement.x + otherRobot.x, movement.y + otherRobot.y, false);
    } else {
        return obj;
    }
}   
