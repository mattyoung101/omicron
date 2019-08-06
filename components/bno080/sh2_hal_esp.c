// SH2 HAL implentation for the ESP32
// By Matt Young, 2019
// Based on the Hillcrest Nucleo demo: https://git.io/fjH9C
#include "sh2_hal.h"
#include "sh2_hal_impl.h"
#include "freertos/semaphore.h"

static SemaphoreHandle_t blockSem = NULL;

void sh2_hal_init(void){
    // initialise stuff like semaphores etc
    blockSem = xSemaphoreCreateBinary();

    // Put SH-2 device into reset

    // Create queue

    // Create receive task
}

int sh2_hal_reset(bool dfuMode, sh2_rxCallback_t *onRx, void *cookie){
    return 0;
}

// Send data to SH-2.
// Call may return without blocking before transfer is complete.
int sh2_hal_tx(uint8_t *pData, uint32_t len){
    return 0;
}

int sh2_hal_rx(uint8_t *pData, uint32_t len){
    return 0;
}

int sh2_hal_block(void){
    return 0;
}

int sh2_hal_unblock(void){
    return 0
}