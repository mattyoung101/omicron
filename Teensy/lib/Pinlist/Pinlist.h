//Header file for defining pins used
#ifndef PINLIST_H
#define PINLIST_H

#include <Arduino.h>
#include <Config.h>

// --- Light Sensors --- //

#define MUX_EN 43
#define MUX_A0 46
#define MUX_A1 34
#define MUX_A2 33
#define MUX_A3 47
#define MUX_A4 48
#define MUX_WR 44
#define MUX_OUT 49

// --- Camera --- //

#define CAM_SERIAL Serial2

// --- Motors --- //

// Motor 1
#define MOTOR_FR_PWM 29
#define MOTOR_FR_IN1 24
#define MOTOR_FR_IN2 25
#define MOTOR_FR_ANGLE 45
#define MOTOR_FR_REVERSED true

// Motor 2
#define MOTOR_BR_PWM 3
#define MOTOR_BR_IN1 2
#define MOTOR_BR_IN2 4
#define MOTOR_BR_ANGLE 135
#define MOTOR_BR_REVERSED false

// Motor 3
#define MOTOR_BL_PWM 6
#define MOTOR_BL_IN1 5
#define MOTOR_BL_IN2 55
#define MOTOR_BL_ANGLE 225
#define MOTOR_BL_REVERSED false

// Motor 4
#define MOTOR_FL_PWM 30
#define MOTOR_FL_IN1 26
#define MOTOR_FL_IN2 27
#define MOTOR_FL_ANGLE 315
#define MOTOR_FL_REVERSED true

// --- LRFs --- //

#define LRF1_RX RX1
#define LRF1_TX TX1

#define LRF2_RX RX2
#define LRF2_TX TX2

#define LRF3_RX RX3
#define LRF3_TX TX3

#define LRF4_RX RX4
#define LRF4_TX TX4

// --- SPI --- //

#define MOSI MOSI
#define MISO MISO
#define SCK SCK
#define CS CS0

// -- I2C --- //

#define IMU_SDA 38
#define IMU_SCL 37
#define IMU_WIRE Wire1

// Wire
#define ESP_SDA 18
#define ESP_SCL 19
#define ESP_WIRE Wire

#define ESPSERIAL Serial3

// --- Debug --- //

#define V_BAT A16
#define V_BAT_LED 36 // A17
// im a afucking idiot again and forgot to add a v_bat led in the first place...

// --- Lightgate --- //

#define LIGHTGATE A0

#endif // PINLIST_H