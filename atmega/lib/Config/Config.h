/*
 * This file is part of the ATMega firmware project.
 * Copyright (c) 2019-2020 Team Omicron. All rights reserved.
 *
 * Team Omicron members: Lachlan Ellis, Tynan Jones, Ethan Lo,
 * James Talkington, Matt Young.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef CONFIG_H
#define CONFIG_H

// --- Code Activation Defines --- //
#define I2C_ON true
#define MOUSE_ON true
#define LED_ON true

// --- I2C --- //
#define I2C_ADDRESS 0x12
#define I2C_START_BYTE 0xB
#define I2C_RCV_PACKET_SIZE 7 // 1 start, 5 data, 1 checksum
#define I2C_SEND_PACKET_SIZE 7 // 1 start, 5 data, 1 checksum

// --- Motors --- //
#define MOTOR_FR_INA 14
#define MOTOR_FR_INB 15
#define MOTOR_FR_PWM 6

#define MOTOR_BR_INA 2
#define MOTOR_BR_INB 4
#define MOTOR_BR_PWM 5

#define MOTOR_BL_INA 7
#define MOTOR_BL_INB 8
#define MOTOR_BL_PWM 9

#define MOTOR_FL_INA 16
#define MOTOR_FL_INB 17
#define MOTOR_FL_PWM 3

#endif
