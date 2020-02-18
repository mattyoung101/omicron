#include "orbit.h"
#include "utils.h"

bool orbit::isInEllipse(Vector robot, float a, float b)
{
    Vector temp1 = Vector(robot.i, robot.j, false);
    float angle = atan2(robot.j, robot.i);
    Vector temp2 = Vector(a * cos(angle), b * sin(angle), false);
    return temp2.mag > temp1.mag;
}
Vector orbit::ellipsePoint(Vector robot, float a, float b)
{
    //fuckin bitch ass i swear to god i will literally shoot you
    float angle = atan2(robot.j, robot.i);
    return Vector(a * cos(angle), b * sin(angle), false);
}

inline float orbit::mCalc(Vector robot, float a, float b)
{
    return (-powf(b, 2) * robot.i) / (powf(a, 2) * robot.j);
}

inline float orbit::cCalc(Vector robot, float a, float b)
{
    return (powf(a, 2) * powf(b, 2)) / (powf(a, 2) * robot.j);
}

inline float orbit::x(Vector robot, float a, float b, int p)
{
    float m = mCalc(robot, a, b);
    float c = cCalc(robot, a, b);
    return ((-powf(a, 2) * m * c) + p * sign(robot.j) * a * b * sqrtf((pow(a, 2) * pow(m, 2)) + pow(b, 2) - pow(c, 2))) / (pow(b, 2) + (pow(a, 2) * pow(m, 2)));
}

inline float orbit::y(Vector robot, float a, float b, int p)
{
    float m = mCalc(robot, a, b);
    float c = cCalc(robot, a, b);
    return ((powf(b, 2) * c) + p * sign(robot.j) * a * b * m * sqrtf((pow(a, 2) * pow(m, 2)) + pow(b, 2) - pow(c, 2))) / (pow(b, 2) + (pow(a, 2) * pow(m, 2)));
}

Vector orbit::calcAvoid(float a, float b, Vector obj, Vector robot)
{
    Vector pointOne = Vector(x(robot, a, b, -1), y(robot, a, b, -1), false);
    Vector pointTwo = Vector(x(robot, a, b, 1), y(robot, a, b, 1), false);

    if (isInEllipse(robot, a, b))
    {
        return ellipsePoint(robot, a, b);
    }
    else if (isAngleBetween(obj.arg, pointTwo.arg, pointOne.arg))
    {
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

Vector orbit::avoidMethod(Vector avoid, float a, float b, Vector obj, Vector robot)
{
    Vector movement = calcAvoid(a, b, obj, Vector(robot.i - avoid.i, robot.j - avoid.j, false));
    return Vector(movement.i + avoid.i, movement.j + avoid.j, false);
}
