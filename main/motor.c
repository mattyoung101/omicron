#include "motor.h"

static float pwmValues[4] = {0};
static float flmotor_speed = 0;
static float frmotor_speed = 0;
static float blmotor_speed = 0;
static float brmotor_speed = 0;
// static const char *TAG = "Motor";

void motor_calc(int16_t direction, int16_t orientation, float speed){
    float radAngle = DEG_RAD * ((float) direction + 180.0f);

    pwmValues[0] = cosf(((MOTOR_FL_ANGLE + 90.0f) * DEG_RAD) - radAngle);
    pwmValues[1] = cosf(((MOTOR_FR_ANGLE + 90.0f) * DEG_RAD) - radAngle);
    pwmValues[2] = cosf(((MOTOR_BL_ANGLE + 90.0f) * DEG_RAD) - radAngle);
    pwmValues[3] = cosf(((MOTOR_BR_ANGLE + 90.0f) * DEG_RAD) - radAngle);

    flmotor_speed = speed * pwmValues[0] + orientation;
    frmotor_speed = speed * pwmValues[1] + orientation;
    blmotor_speed = speed * pwmValues[2] + orientation;
    brmotor_speed = speed * pwmValues[3] + orientation;

    float maxSpeed = fmaxf(
        fmaxf(fabsf(flmotor_speed), fabsf(frmotor_speed)), 
        fmaxf(fabsf(blmotor_speed), fabsf(brmotor_speed))
        );

    flmotor_speed = speed == 0 ? flmotor_speed : (flmotor_speed / maxSpeed) * speed;
    frmotor_speed = speed == 0 ? frmotor_speed : (frmotor_speed / maxSpeed) * speed;
    blmotor_speed = speed == 0 ? blmotor_speed : (blmotor_speed / maxSpeed) * speed;
    brmotor_speed = speed == 0 ? brmotor_speed : (brmotor_speed / maxSpeed) * speed;
}

// ... rest of the motor controller code was removed as it runs on the Teensy instead ...