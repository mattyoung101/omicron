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

// Handles slave to master communication over I2C
// TODO: We're in Open why do we have tsop stuff :/

typedef struct {
    uint16_t tsopAngle;
    uint16_t tsopStrength;
    uint16_t lineAngle;
    uint16_t lineSize;
    uint16_t heading;
} i2c_data_t;

typedef struct {
    int16_t mouseDX;
    int16_t mouseDY;
} nano_data_t;

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