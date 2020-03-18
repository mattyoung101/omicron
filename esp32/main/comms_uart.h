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
#include "driver/uart.h"
#include "pb_decode.h"
#include "UART.pb.h"
#include "wirecomms.pb.h"

// Handles communication between different devices over UART
// message format is: [0xB, msgId, msgSize, ...PROTOBUF DATA..., CRC8_checksum, 0xE]

/** enum of different UART endpoints, or physical devices */
typedef enum {
    SBC_CAMERA = 0,
    MCU_TEENSY,
} uart_endpoint_t;

extern ObjectData lastObjectData;
extern LocalisationData lastLocaliserData;
extern ESP32DebugCommand lastDebugCmd;
extern LSlaveToMaster lastLSlaveData;
extern SemaphoreHandle_t uartDataSem;
extern SemaphoreHandle_t validCamPacket;

/** Initialises UART on the given port */
void comms_uart_init(uart_endpoint_t device);
/** Sends data to the specified endpoint without waiting for the response */
esp_err_t comms_uart_send(uart_endpoint_t device, msg_type_t msgId, uint8_t *pbData, size_t msgSize);
/** Sends a message without any Protobuf content */
esp_err_t comms_uart_notify(uart_endpoint_t device, msg_type_t msgId);