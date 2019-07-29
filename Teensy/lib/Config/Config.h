// Global config
#ifndef CONFIG_H
#define CONFIG_H

// --- Light Sensors --- //

#define LS_NUM 32
#define DEBUG_DATA false
#define DEBUG_RAW false

#define LS_CALIBRATION_COUNT 10
#define LS_CALIBRATION_BUFFER 100
#define LS_ES_DEFAULT 69
#define NO_LINE_ANGLE 400
#define NO_LINE_SIZE 400
#define LS_NUM_MULTIPLIER 11.25
#define LS_LINEOVER_BUFFER 90

// --- IMU --- //

#define IMU_CALIBRATION_COUNT 15
#define IMU_CALIBRATION_TIME 150
#define IMU_THRESHOLD 1000

#define MPU9250_ADDRESS 0x68
#define MAG_ADDRESS 0x0C

#define GYRO_FULL_SCALE_250_DPS 0x00
#define GYRO_FULL_SCALE_500_DPS 0x08
#define GYRO_FULL_SCALE_1000_DPS 0x10
#define GYRO_FULL_SCALE_2000_DPS 0x18

#define ACC_FULL_SCALE_2_G 0x00
#define ACC_FULL_SCALE_4_G 0x08
#define ACC_FULL_SCALE_8_G 0x10
#define ACC_FULL_SCALE_16_G 0x18

// --- Battery Monitor -- //

#define V_REF 3.3
#define R1 1000000
#define R2 300000
#define V_BAT_MIN 11.1

// --- Math --- //

#define DEG_RAD 0.017453292519943295 // multiply to convert degrees to radians
#define RAD_DEG 57.29577951308232 // multiply to convert radians to degrees
// #define PI 3.141592653589793238462643383279502884197169399375105820974944592307816406286
#define MATH_E 2.7182818284590452353602874713527

#endif // CONFIG_H