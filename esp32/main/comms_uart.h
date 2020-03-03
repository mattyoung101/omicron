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

// Handles slave to master communication over UART
#define UART_BUF_SIZE 128

typedef enum {
    SBC_CAMERA = 0,
    MCU_TEENSY = 1,
} uart_dev_type_t;

typedef struct {
    uart_dev_type_t deviceType;
} uart_endpoint_t;

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