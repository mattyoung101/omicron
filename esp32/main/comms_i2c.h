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
#include "i2c.pb.h"

// Handles slave to master communication over I2C

typedef struct {
    uint16_t tsopAngle;
    uint16_t tsopStrength;
    uint16_t lineAngle;
    uint16_t lineSize;
    uint16_t heading;
} i2c_data_t;

typedef struct {
    float lineAngle;
    float lineSize;
    bool isOnLine;
    bool isLineOver;
    float lastAngle;
    float batteryVoltage;
} nano_data_t;

/** Initialises I2C. **/
void comms_i2c_init(i2c_port_t port);
/** Sends data to the slave without waiting for the response */
esp_err_t comms_i2c_send(msg_type_t msgId, uint8_t *pbData, size_t msgSize);
/** Sends a message without any Protobuf content */
esp_err_t comms_i2c_notify(msg_type_t msgId);
/** Hack to work around sending data for States comp, sends Protobuf stuff then receives a 16 bit int for heading */
esp_err_t comms_i2c_workaround(msg_type_t msgId, uint8_t *pbData, size_t msgSize);