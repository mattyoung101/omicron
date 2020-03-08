#include "comms_uart.h"

static const char *TAG = "CommsUART";
uart_data_t receivedData = {0};

// UART comms, between Teensy and ESP32
// Could possibly also recycle to use with the camera since they both use Protobuf, but we'd have to change it a bit
// because camera stuff needs to take action after decoding. But may not be the worst idea actually.

static void uart_receive_task(void *pvParameter){
    static const char *TAG = "CamReceiveTask";;
    
    esp_task_wdt_add(NULL);
    ESP_LOGI(TAG, "UART receive task init OK!");

    while (true){
        // first let's read in the header and see what message id, and how much we need to read in
        // message format is: [0xB, msg_type, size, ...data..., 0xE] - so we have a 3 byte header
        uint8_t header[3] = {0};
        uart_read_bytes(UART_NUM_1, header, 3, portMAX_DELAY);
        esp_task_wdt_reset();

        if (header[0] == 0xB){
            msg_type_t msgType = header[1];
            uint8_t msgSize = header[2];

            // read in the rest of the data
            uint8_t data[msgSize];
            uart_read_bytes(UART_NUM_1, data, msgSize, pdMS_TO_TICKS(250));

            // decode with protobuf
            pb_istream_t stream = pb_istream_from_buffer(data, msgSize);
            // TODO write up the decoding stuff here, and also probably want to consider message id as a thing
            // we need to come up with a clean way to get the protobuf uart files into the esp32 directory

            esp_task_wdt_reset();
            uart_flush_input(UART_NUM_1);
        } else {
            ESP_LOGW(TAG, "Received invalid UART packet, begin byte was 0x%X not 0xB", header[0]);
        }
    }
}

void comms_uart_init(void){
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    // Configure UART parameters
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_1, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1, 18, 18, -1, -1)); // TODO fix pins
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1, 256, 256, 8, NULL, 0));

    xTaskCreate(uart_receive_task, "UARTReceiveTask", 4096, NULL, configMAX_PRIORITIES - 1, NULL);
    ESP_LOGI(TAG, "UART comms init OK!");
}

esp_err_t comms_uart_send(msg_type_t msgId, uint8_t *pbData, size_t msgSize){
    if (msgSize > UINT8_MAX){
        ESP_LOGW(TAG, "Message too big to send properly, size is: %zu", msgSize);
    }

    char header[] = {0xB, msgId, msgSize};
    char end = 0xE;

    uart_write_bytes(UART_NUM_1, header, 3);
    uart_write_bytes(UART_NUM_1, (char*) pbData, msgSize);
    uart_write_bytes(UART_NUM_1, &end, 1);

    return ESP_OK;
}

esp_err_t comms_uart_notify(msg_type_t msgId){
    // not really implemented (as it's not needed as of now)
    return ESP_FAIL;
}