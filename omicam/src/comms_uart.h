#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

/** Initialises the UART TTY to the given baud rate */
void comms_uart_init();
/** Sends the data of the specified size over UART. comms_uart_init must be called first */
void comms_uart_send(uint8_t *data, size_t size);
/** Closes the UART TTY */
void comms_uart_dispose();