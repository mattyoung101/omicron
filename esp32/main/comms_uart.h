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
#include "driver/uart.h"

// Handles slave to master communication over UART

typedef struct {
    uint16_t tsopAngle;
    uint16_t tsopStrength;
    uint16_t lineAngle;
    uint16_t lineSize;
    uint16_t heading;
} uart_data_t;

extern uart_data_t receivedData;

/** Initialises UART on the given port **/
void comms_uart_init(void);
/** Sends data to the slave without waiting for the response */
esp_err_t comms_uart_send(msg_type_t msgId, uint8_t *pbData, size_t msgSize);
/** Sends a message without any Protobuf content */
esp_err_t comms_uart_notify(msg_type_t msgId);