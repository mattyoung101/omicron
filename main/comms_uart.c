#include "comms_uart.h"

static const char *TAG = "Comms_UART";
uart_data_t receivedData = {0};

static void uart_receive_task(void *pvParameter){
    ESP_LOGI(TAG, "UART receive task init OK!");

    while (true){
        // TODO receive bytes here - not used currently
        vTaskSuspend(NULL);
    }
}

void comms_uart_init(void){
    /*
    TODO do we need to do HW flow control?
    */
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    // Configure UART parameters
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_1, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1, 21, 22, -1, -1)); // 21 == TX == SDA, 22 == RX == SCL
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1, 256, 256, 8, NULL, 0));

    xTaskCreate(uart_receive_task, "UARTReceiveTask", 4096, NULL, configMAX_PRIORITIES - 1, NULL);
    ESP_LOGI(TAG, "UART comms init OK!");
}

esp_err_t comms_uart_send(msg_type_t msgId, uint8_t *pbData, size_t msgSize){
    char header[] = {0xB, msgId, msgSize};
    char end = 0xEE;

    uart_write_bytes(UART_NUM_1, header, 3);
    uart_write_bytes(UART_NUM_1, (char*) pbData, msgSize);
    uart_write_bytes(UART_NUM_1, &end, 1);

    return ESP_OK;
}

esp_err_t comms_uart_notify(msg_type_t msgId){
    // not really implemented (as it's not needed as of now)
    return ESP_FAIL;
}