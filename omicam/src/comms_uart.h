/*
 * This file is part of the Omicam project.
 * Copyright (c) 2019-2020 Team Omicron. All rights reserved.
 *
 * Team Omicron members: Lachlan Ellis, Tynan Jones, Ethan Lo,
 * James Talkington, Matt Young.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "protobuf/UART.pb.h"
#include "defines.h"


#ifdef __cplusplus
extern "C" {
#endif

extern SensorData lastSensorData;
extern double lastUARTReceiveTime;

/** Initialises the UART TTY at a baud rate of 115200, which is the standard JimBus baud rate */
void comms_uart_init();
/**
 * Sends the data of the specified size over UART. comms_uart_init() must be called first.
 *
 * Data is sent in the JimBus format, which is designed by us (Team Omicron) to efficiently send Protobuf messages between
 * devices using UART. Omicam does not support the I2C version, JimBusLE, because it only communicates over UART.
 * Each packet can contain 1 protobuf message, with up to 255 IDs supported, and a max Protobuf size of 255 bytes.
 * Message format: [0xB, msgId, msgSize, ...PROTOBUF DATA..., CRC8 checksum, 0xE]
 *
 * IMPORTANT: This function IS thread safe. If two threads call comms_uart_send() at the same time, a mutex is used
 * to sync the bus so that the messages don't thrash with each other.
 */
void comms_uart_send(comms_msg_type_t msgId, uint8_t *data, size_t size);
/** Closes the UART TTY */
void comms_uart_dispose();

#ifdef __cplusplus
};
#endif