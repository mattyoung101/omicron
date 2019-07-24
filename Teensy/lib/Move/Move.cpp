// By Ethan Lo

#include "Arduino.h"
#include "Move.h"
#include <Pinlist.h>
#include <Config.h>

void Move::set()
{
    pinMode(MOTOR_FL_PWM, OUTPUT);
    pinMode(MOTOR_FL_IN1, OUTPUT);
    pinMode(MOTOR_FL_IN2, OUTPUT);
    pinMode(MOTOR_FR_PWM, OUTPUT);
    pinMode(MOTOR_FR_IN1, OUTPUT);
    pinMode(MOTOR_FR_IN2, OUTPUT);
    pinMode(MOTOR_BL_PWM, OUTPUT);
    pinMode(MOTOR_BL_IN1, OUTPUT);
    pinMode(MOTOR_BL_IN2, OUTPUT);
    pinMode(MOTOR_BR_PWM, OUTPUT);
    pinMode(MOTOR_BR_IN1, OUTPUT);
    pinMode(MOTOR_BR_IN2, OUTPUT);
}
void Move::motorCalc(int angle, int dir, int speed)
{
    radAngle = radians(angle);

    values[0] = cos((radians(MOTOR_FL_ANGLE + 90)) - radAngle);
    values[1] = cos((radians(MOTOR_FR_ANGLE + 90)) - radAngle);
    values[2] = cos((radians(MOTOR_BL_ANGLE + 90)) - radAngle);
    values[3] = cos((radians(MOTOR_BR_ANGLE + 90)) - radAngle);

    flmotor_pwm = speed * values[0] + dir;
    frmotor_pwm = speed * values[1] + dir;
    blmotor_pwm = speed * values[2] + dir;
    brmotor_pwm = speed * values[3] + dir;

    double max_speed = max(max(abs(flmotor_pwm),abs(frmotor_pwm)),max(abs(blmotor_pwm),abs(brmotor_pwm)));

    flmotor_pwm = speed == 0 ? flmotor_pwm : (flmotor_pwm/max_speed)*speed;
    frmotor_pwm = speed == 0 ? frmotor_pwm : (frmotor_pwm/max_speed)*speed;
    blmotor_pwm = speed == 0 ? blmotor_pwm : (blmotor_pwm/max_speed)*speed;
    brmotor_pwm = speed == 0 ? brmotor_pwm : (brmotor_pwm/max_speed)*speed;
}

void Move::move(int speed, int inOnePin, int inTwoPin, int pwmPin, bool reversed, bool brake)
{
	if (speed > 0) {
		analogWrite(pwmPin, constrain(speed, 0, 255));

		if (reversed) {
			digitalWrite(inOnePin, HIGH);
			digitalWrite(inTwoPin, LOW);
		} else {
			digitalWrite(inOnePin, LOW);
			digitalWrite(inTwoPin, HIGH);
		}
	} else if (speed < 0) {
		analogWrite(pwmPin, constrain(abs(speed), 0, 255));

		if (reversed) {
			digitalWrite(inOnePin, LOW);
			digitalWrite(inTwoPin, HIGH);
		} else {
			digitalWrite(inOnePin, HIGH);
			digitalWrite(inTwoPin, LOW);
		}
	} else {
        if(brake){
            digitalWrite(inOnePin, LOW);
            digitalWrite(inTwoPin, LOW);
            digitalWrite(pwmPin, HIGH);
        }else{
            digitalWrite(inOnePin, HIGH);
            digitalWrite(inTwoPin, HIGH);
            digitalWrite(pwmPin, HIGH);
        }
    
	}
}

void Move::go(bool brake)
{
    move(flmotor_pwm, MOTOR_FL_IN1, MOTOR_FL_IN2, MOTOR_FL_PWM, MOTOR_FL_REVERSED, brake);
    move(frmotor_pwm, MOTOR_FR_IN1, MOTOR_FR_IN2, MOTOR_FR_PWM, MOTOR_FR_REVERSED, brake);
    move(blmotor_pwm, MOTOR_BL_IN1, MOTOR_BL_IN2, MOTOR_BL_PWM, MOTOR_BL_REVERSED, brake);
    move(brmotor_pwm, MOTOR_BR_IN1, MOTOR_BR_IN2, MOTOR_BR_PWM, MOTOR_BR_REVERSED, brake);
}

void Move::motorTest(int pwm){
    move(pwm, MOTOR_FL_IN1, MOTOR_FL_IN2, MOTOR_FL_PWM, MOTOR_FL_REVERSED, false);
    move(pwm, MOTOR_FR_IN1, MOTOR_FR_IN2, MOTOR_FR_PWM, MOTOR_FR_REVERSED, false);
    move(pwm, MOTOR_BL_IN1, MOTOR_BL_IN2, MOTOR_BL_PWM, MOTOR_BL_REVERSED, false);
    move(pwm, MOTOR_BR_IN1, MOTOR_BR_IN2, MOTOR_BR_PWM, MOTOR_BR_REVERSED, false);
}
