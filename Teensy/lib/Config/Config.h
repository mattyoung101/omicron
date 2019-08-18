// GLOBAL CONFIG
#ifndef CONFIG_H
#define CONFIG_H

// --- Code Activation Defines --- //
#define ESP_I2C_ON false
#define MPU_I2C_ON true
#define CALIBRATE_MPU true
#define CAM_ON true
#define LS_ON true
#define LRFS_ON false
#define LED_ON true
#define YEET_TIMER 4000
// --- Game Settings --- //
#define DEFENCE false
#define ENEMY_GOAL 0 //0: Yellow, 1: Blue, 2: OFF
#define STARTUP_DELAY 200

// --- Acceleration Settings --- //
#define ACCEL_TIME_STEP 1000
#define ACCEL_PROGRESS 1

// --- Orbit --- //
#define BALL_FAR_STRENGTH 100
#define BALL_CLOSE_STRENGTH 40
#define ORBIT_SPEED_SLOW 80
#define ORBIT_SPEED_FAST 220
#define BALL_INFRONT_ANGLE 10
#define YEET_SPEED 255
#define GOAL_INFRONT_ANGLE 10
#define BALL_INFRONT_STRENGTH 50

#define GOAL_KP 1.2
#define GOAL_KI 0
#define GOAL_KD 0.3
#define GOAL_MAX_CORRECTION 255

#define HEADING_KP 2.5
#define HEADING_KI 0
#define HEADING_KD 0.2
#define HEADING_MAX_CORRECTION 255

#define IDLE_DISTANCE 100

// --- Defence --- //
#define SIDE_KP 5
#define SIDE_KI 0
#define SIDE_KD 0.3
#define SIDE_MAX 255
#define CENTRE_SIDE_MAX 200

#define FORWARD_KP 10
#define FORWARD_KI 0
#define FORWARD_KD 0.2
#define FORWARD_MAX 255
#define CENTRE_FORWARD_MAX 200

#define GOALIE_KP 4
#define GOALIE_KI 0
#define GOALIE_KD 0.1
#define GOALIE_MAX 255

#define DEFEND_DISTANCE 100
#define SURGE_DISTANCE 150
#define SURGE_STRENGTH 50

// --- Camera Settings --- //

#define CAM_DATA_LENGTH 10
#define CAM_CENTRE_X 115
#define CAM_CENTRE_Y 115
#define CAM_BEGIN_BYTE 0xB
#define CAM_END_BYTE 0xE

// --- Light Sensors --- //

#define LS_NUM 36
#define DEBUG_DATA false
#define DEBUG_RAW false

#define LS_CALIBRATION_COUNT 10
#define LS_CALIBRATION_BUFFER 200
#define LS_ES_DEFAULT 69
#define NO_LINE_ANGLE 400
#define NO_LINE_SIZE -100
#define LS_NUM_MULTIPLIER 10
#define LS_LINEOVER_BUFFER 90

#define OVER_LINE_SPEED 255
#define LINE_SPEED 255
#define LINE_SPEED_MULTIPLIER 0.7
#define LINE_TRACK_SPEED 50

#define LINE_SMALL_SIZE 0
#define LINE_BIG_SIZE 0
// --- IMU --- //

#define IMU_CALIBRATION_COUNT 100
#define IMU_CALIBRATION_TIME 10
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

// --- Battery Monitor --- //

#define V_REF 3.3
#define R1 1000000
#define R2 300000
#define V_BAT_MIN 11.1

// --- Other --- //

#define IDLE_MIN_SPEED 10

// --- Math --- //

#define DEG_RAD 0.017453292519943295 // multiply to convert degrees to radians
#define RAD_DEG 57.29577951308232 // multiply to convert radians to degrees
// #define PI 3.141592653589793238462643383279502884197169399375105820974944592307816406286
#define MATH_E 2.7182818284590452353602874713527

#endif // CONFIG_H