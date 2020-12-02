/*
 * This file is part of the ESP32 firmware project.
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
#include "driver/i2c.h"
#include "defines.h"
#include "alloca.h"
#include "utils.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "string.h"
#include "esp_task_wdt.h"
#include "wirecomms.pb.h"

// Handles I2C comms to the BNO-055 and also the ATMega328P (Nano)
// I2C comms to the Nano are implemented using JimBusLE (JimBus Lite Edition), which is similar to the full-featured
// UART JimBus protocol, except that it sends bitshifted data not Protobufs (because Nanopb is thought to be too
// expensive to run on the ATMega), and also doesn't encode a message id.
// JimBusLE message format: [0xB, ]
// TODO: We're in Open why do we have tsop stuff :/

typedef struct {
    int16_t mouseDX;
    int16_t mouseDY;
} nano_data_t;

extern nano_data_t nanoData;

/** Initialises BNO I2C port. **/
void comms_i2c_init_bno(i2c_port_t port);
/** Initialises atmega I2C port. **/
void comms_i2c_init_nano(i2c_port_t port);
/** Sends data to the slave without waiting for the response */
esp_err_t comms_i2c_send(msg_type_t msgId, uint8_t *pbData, size_t msgSize);
/** Sends a message without any Protobuf content */
esp_err_t comms_i2c_notify(msg_type_t msgId);
/** Hack to work around sending data for States comp, sends Protobuf stuff then receives a 16 bit int for heading */
esp_err_t comms_i2c_workaround(msg_type_t msgId, uint8_t *pbData, size_t msgSize);