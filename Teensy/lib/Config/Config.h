#ifndef CONFIG_H
#define CONFIG_H

#include "Arduino.h"

// --- Code Activation Defines --- //
#define LS_ON true
#define LRFS_ON true
#define LED_ON true

// --- Light Sensors --- //
#define LS_NUM 32
#define DEBUG_DATA false
#define DEBUG_RAW false
#define DEBUG_FILLED false

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

// and new versions:

#define FIRST_CALIB_REGISTER 500
#define LS_RING_NUM 32

// --- LRFs --- //
#define LRF_DATA_LENGTH 8
#define LRF_START_BYTE 0x5A

#define LRF1_SERIAL Serial1
#define LRF2_SERIAL Serial2
#define LRF3_SERIAL Serial3
#define LRF4_SERIAL Serial4

// --- Math --- //
#define DEG_RAD 0.017453292519943295 // multiply to convert degrees to radians
#define RAD_DEG 57.29577951308232 // multiply to convert radians to degrees
// #define PI 3.141592653589793238462643383279502884197169399375105820974944592307816406286
#define MATH_E 2.7182818284590452353602874713527

#endif // CONFIG_H