#ifndef CONFIG_H
#define CONFIG_H

// --- Code Activation Defines --- //
#define I2C_ON true
#define MOUSE_ON true

// --- I2C --- //
#define I2C_ADDRESS 0x12
#define I2C_START_BYTE 0xB
#define I2C_PACKET_SIZE 9

// --- Motors --- //
#define MOTOR_FR_INA 14
#define MOTOR_FR_INB 15
#define MOTOR_FR_PWM 6

#define MOTOR_BR_INA 16
#define MOTOR_BR_INB 17
#define MOTOR_BR_PWM 5

#define MOTOR_BL_INA 4
#define MOTOR_BL_INB 7
#define MOTOR_BL_PWM 9

#define MOTOR_FL_INA 2
#define MOTOR_FL_INB 8
#define MOTOR_FL_PWM 3

#endif