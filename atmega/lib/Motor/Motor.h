#ifndef MOTOR_H
#define MOTOR_H

class Motor{
    public:
        void init();
        void run(int PWM, int inA, int inB, int pwmPin);
        void move(int frPWM, int brPWM, int blPWM, int flPWM);
};

#endif