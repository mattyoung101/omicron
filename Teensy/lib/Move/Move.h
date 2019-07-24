#ifndef Move_h
#define Move_h

#include "Arduino.h"
#include <Pinlist.h>

class Move
{
public:
    void motorCalc(int angle, int dir, int speed);
    void set();
    void move(int speed, int inOnePin, int inTwoPin, int pwmPin, bool reversed, bool brake);
    void go(bool brake);

    void motorTest(int pwm);

    double flmotor_pwm;
    double frmotor_pwm;
    double blmotor_pwm;
    double brmotor_pwm;
private:
    double values[4];
    double radAngle;
};

#endif
