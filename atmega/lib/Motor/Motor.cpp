#include "Motor.h"
#include "Config.h"
#include "Arduino.h"

void Motor::init(){
    pinMode(MOTOR_FL_PWM, OUTPUT);
    pinMode(MOTOR_FL_INA, OUTPUT);
    pinMode(MOTOR_FL_INB, OUTPUT);
    pinMode(MOTOR_FR_PWM, OUTPUT);
    pinMode(MOTOR_FR_INA, OUTPUT);
    pinMode(MOTOR_FR_INB, OUTPUT);
    pinMode(MOTOR_BL_PWM, OUTPUT);
    pinMode(MOTOR_BL_INA, OUTPUT);
    pinMode(MOTOR_BL_INB, OUTPUT);
    pinMode(MOTOR_BR_PWM, OUTPUT);
    pinMode(MOTOR_BR_INA, OUTPUT);
    pinMode(MOTOR_BR_INB, OUTPUT);
}

void Motor::run(int PWM, int inA, int inB, int pwmPin){
    if(PWM == 0){
        digitalWrite(inA, HIGH);
        digitalWrite(inB, HIGH);
    } else {
        digitalWrite(inA, PWM < 0 ? LOW : HIGH);
        digitalWrite(inB, PWM < 0 ? HIGH : LOW);
    }
    analogWrite(pwmPin, abs(PWM));
}

void Motor::move(int frPWM, int brPWM, int blPWM, int flPWM){
    run(frPWM, MOTOR_FR_INA, MOTOR_FR_INB, MOTOR_FR_PWM);
    run(brPWM, MOTOR_BR_INA, MOTOR_BR_INB, MOTOR_BR_PWM);
    run(blPWM, MOTOR_BL_INA, MOTOR_BL_INB, MOTOR_BL_PWM);
    run(flPWM, MOTOR_FL_INA, MOTOR_FL_INB, MOTOR_FL_PWM);
}
