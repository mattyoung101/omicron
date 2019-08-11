// SH2 HAL implentation for the ESP32
// By Matt Young, 2019
// Based on the Hillcrest Nucleo demo: https://git.io/fjH9C
#include "sh2_hal.h"
#include "sh2_hal_impl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "sh2_err.h"
#include "driver/i2c.h"
#include "esp_err.h"

#define DFU_BOOT_DELAY (200) // [mS]
#define RESET_DELAY    (10) // [mS]

#define MAX_EVENTS (16)
#define SHTP_HEADER_LEN (4)

#define ADDR_SH2_0 (0x4A)
#define ADDR_SH2_1 (0x4B)

static SemaphoreHandle_t blockSem = NULL;
static QueueHandle_t eventQueue = NULL;
static const char *TAG = "BNO080_HAL";
static void *onRxCookie = NULL;
static sh2_rxCallback_t *onRxCallback = NULL;
static uint8_t addr = 0;
static size_t rxRemaining = 0;
static uint8_t rxBuf[SH2_HAL_MAX_TRANSFER] = {0};

// they implemented this really weirdly with these structs but we'll keep it
// because the onRx event handler demands this info (iirc)
typedef enum {
    EVT_INTN,
} EventId_t;

typedef struct {
    uint32_t t_ms;
    EventId_t id;
} Event_t;

static int i2c_rx(uint8_t addr, uint8_t *pData, size_t len){
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    ESP_ERROR_CHECK(i2c_master_start(cmd));
    // first, send device address (indicating write) & register to be read
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (addr << 1), I2C_ACK_MODE));
    // Send repeated start
    ESP_ERROR_CHECK(i2c_master_start(cmd));
    // now send device address (indicating read) & read data
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_READ, I2C_ACK_MODE));
    if (len > 1) {
        ESP_ERROR_CHECK(i2c_master_read(cmd, pData, len - 1, 0x0));
    }
    ESP_ERROR_CHECK(i2c_master_read_byte(cmd, pData + len - 1, 0x1));
    ESP_ERROR_CHECK(i2c_master_stop(cmd));
    esp_err_t err = i2c_master_cmd_begin(BNO_BUS, cmd, pdMS_TO_TICKS(I2C_TIMEOUT));
    i2c_cmd_link_delete(cmd);

    if (err != ESP_OK){
        ESP_LOGE(TAG, "I2C error in i2c_rx (sh2_hal_esp): %s", esp_err_to_name(err));
        return SH2_ERR_IO;
    }
    ESP_LOGI(TAG, "read successful!");
    return SH2_OK;
}

static void sensor_task(void *pvParams){
    Event_t event = {0};
    size_t readLen = 0;
    size_t cargoLen = 0;

    ESP_LOGI(TAG, "Sensor task init OK!");

    while (true) {
        xQueueReceive(eventQueue, &event, portMAX_DELAY);
        ESP_LOGD(TAG, "new event from queue");

        switch (event.id) {
            case EVT_INTN:
                if (onRxCallback != 0) {
                    readLen = rxRemaining;

                    ESP_LOGD(TAG, "readLen before fixing: %d", readLen);
                    
                    if (readLen < SHTP_HEADER_LEN) {
                        // always read at least the SHTP header
                        readLen = SHTP_HEADER_LEN;
                    }
                    if (readLen > SH2_HAL_MAX_TRANSFER) {
                        // limit reads to transfer size
                        readLen = SH2_HAL_MAX_TRANSFER;
                    }

                    ESP_LOGD(TAG, "invoking read on addr 0x%X with %d bytes", addr, readLen);

                    i2c_rx(addr, rxBuf, readLen);
                    ESP_LOG_BUFFER_HEXDUMP(TAG, rxBuf, readLen, ESP_LOG_INFO);

                    // Get total cargo length from SHTP header
                    cargoLen = ((rxBuf[1] << 8) + (rxBuf[0])) & (~0x8000);
                
                    // Re-evaluate rxRemaining
                    if (cargoLen > readLen) {
                        ESP_LOGD(TAG, "more to read");
                        // More to read.
                        rxRemaining = (cargoLen - readLen) + SHTP_HEADER_LEN;
                    }
                    else {
                        ESP_LOGD(TAG, "read completed");
                        // All done, next read should be header only.
                        rxRemaining = 0;
                    }

                    ESP_LOGD(TAG, "delivering callback");

                    // Deliver via onRxCallback callback
                    onRxCallback(onRxCookie, rxBuf, readLen, event.t_ms * 1000);
                }
                break;
            default:
                break;
        }
    }
}

// TODO check its the right pin?
static void IRAM_ATTR gpio_isr(void *arg){
    Event_t event = {0};
    BaseType_t woken = pdFALSE;

    event.t_ms = xTaskGetTickCountFromISR();
    event.id = EVT_INTN;
    xQueueSendFromISR(eventQueue, &event, &woken);
    
    ets_printf("---> ISR invoked, t_ms=%d\n", event.t_ms);

    if (woken) portYIELD_FROM_ISR();
}

void sh2_hal_init(void){
    ESP_LOGD(TAG, "Initialising BNO-080 HAL...");

    // initialise stuff like semaphores etc
    blockSem = xSemaphoreCreateBinary();
    eventQueue = xQueueCreate(8, sizeof(Event_t));
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BNO_INTN_PIN, gpio_isr, 0);
    // the docs say that the INTN pin is active low meaning the interrupt goes off when the pin goes low
    // (it's usually high) so that means we're looking for the falling edge
    gpio_set_intr_type(BNO_INTN_PIN, GPIO_INTR_NEGEDGE);

    // Put SH-2 device into reset
    gpio_set_level(BNO_RSTN_PIN, 0); // hold in reset

    // Create receive task etc
    xTaskCreate(sensor_task, "SensorTask", 4096, NULL, configMAX_PRIORITIES - 2, NULL);
    ESP_LOGI(TAG, "BNO080 HAL init OK!");
}

int sh2_hal_reset(bool dfuMode, sh2_rxCallback_t *onRx, void *cookie){
    ESP_LOGD(TAG, "Resetting BNO080...");
    onRxCookie = cookie;
    onRxCallback = onRx;
    addr = ADDR_SH2_1; // it appears to be address 1 instead of address 0

    // enable reset line, we don't use DFU so we always set it high
    gpio_set_level(BNO_RSTN_PIN, 0);

    // wait for reset to take effect
    vTaskDelay(pdMS_TO_TICKS(250));

    // disable reset line
    gpio_set_level(BNO_RSTN_PIN, 1);

    // reset I2C needed???

    ESP_LOGD(TAG, "Reset OK!");

    return SH2_OK;
}

int sh2_hal_tx(uint8_t *pData, uint32_t len){
    ESP_LOGI(TAG, "sh2_hal_tx");
    if (len == 0) return SH2_OK;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    ESP_ERROR_CHECK(i2c_master_start(cmd));

    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, I2C_ACK_MODE));
    ESP_ERROR_CHECK(i2c_master_write(cmd, pData, len, I2C_ACK_MODE));
    ESP_ERROR_CHECK(i2c_master_stop(cmd));

    esp_err_t err = i2c_master_cmd_begin(BNO_BUS, cmd, pdMS_TO_TICKS(I2C_TIMEOUT));
    i2c_cmd_link_delete(cmd);

    if (err != ESP_OK){
        ESP_LOGE(TAG, "I2C error in sh2_hal_tx: %s", esp_err_to_name(err));
        return SH2_ERR_IO;
    }
    return SH2_OK;
}

int sh2_hal_rx(uint8_t *pData, uint32_t len){
    ESP_LOGI(TAG, "sh2_hal_rx");
    if (len == 0) return SH2_OK;
    return i2c_rx(addr, pData, len);
}


// TODO i2c rx and tx is blocking so in theory we shouldn't have to block here?

int sh2_hal_block(void){
    // ESP_LOGI(TAG, "sh2_hal_block");
    // xSemaphoreTake(blockSem, portMAX_DELAY);
    return SH2_OK;
}

int sh2_hal_unblock(void){
    // ESP_LOGI(TAG, "sh2_hal_unblock");
    // xSemaphoreGive(blockSem);
    return SH2_OK;
}