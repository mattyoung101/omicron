#include "comms_uart.h"

static const char *TAG = "JimBus";
ObjectData lastObjectData = ObjectData_init_zero;
LocalisationData lastLocaliserData = LocalisationData_init_zero;
ESP32DebugCommand lastDebugCmd = ESP32DebugCommand_init_zero;
LSlaveToMaster lastLSlaveData = LSlaveToMaster_init_zero;

SemaphoreHandle_t uartDataSem = NULL;
SemaphoreHandle_t validCamPacket = NULL;
static bool createdSemaphore = false;

// JimBus comms implementation

static void uart_receive_task(void *pvParameter){
    static const char *TAG = "JimBusRX";

    esp_task_wdt_add(NULL);
    uart_endpoint_t device = (uart_endpoint_t) pvParameter;
    uart_port_t port = device == SBC_CAMERA ? UART_NUM_2 : UART_NUM_1;
    ESP_LOGI(TAG, "UART receive task init OK on endpoint: %d", device);

    while (true){
        // check if we have a begin byte available
        // ESP_LOGD(TAG, "device: %d", device);
        uint8_t byte = 0;
        uart_read_bytes(port, &byte, 1, portMAX_DELAY);
        if (byte != 0xB){
            continue;
        } else if (device == SBC_CAMERA){
            // ESP_LOGD(TAG, "got: 0x%.2X", byte);
        }

        // read in the rest of the header: message id and size
        uint8_t header[2] = {0};
        uart_read_bytes(port, header, 2, pdMS_TO_TICKS(250));
        msg_type_t msgType = header[0];
        uint8_t msgSize = header[1];

        // read in the rest of the data
        uint8_t data[msgSize];
        uart_read_bytes(port, data, msgSize, pdMS_TO_TICKS(250));

        // read in and verify checksum, if it fails flush the entire buffer for this device and try again
        uint8_t receivedChecksum = 0;
        uint8_t actualChecksum = crc8(data, msgSize);
        uart_read_bytes(port, &receivedChecksum, 1, pdMS_TO_TICKS(250));

        if (receivedChecksum != actualChecksum){
            ESP_LOGW(TAG, "Data integrity checked failed on device: %d, expected 0x%.2X, got 0x%.2X", 
                    device, actualChecksum, receivedChecksum);
            uart_flush_input(port);
            continue;
        } else {
            // ESP_LOGI(TAG, "Data integrity check passed: %d!", device);
        }

        // set up what fields and destination structs we need to decode
        pb_istream_t stream = pb_istream_from_buffer(data, msgSize);
        const pb_field_t *fields = NULL;
        void *dest = NULL;

        if (device == SBC_CAMERA){
            switch (msgType){
                case OBJECT_DATA:
                    fields = ObjectData_fields;
                    dest = &lastObjectData;
                    break;
                case LOCALISATION_DATA:
                    fields = LocalisationData_fields;
                    dest = &lastLocaliserData;
                    break;
                case DEBUG_CMD:
                    fields = ESP32DebugCommand_fields;
                    dest = &lastDebugCmd;
                    // TODO depending on contents, notify bluetooth that we have a message to forward
                    break;
                default:
                    ESP_LOGW(TAG, "Unhandled message id %d on device %d, going to drop message", msgType, device);
                    continue;
            }

            // let's just hope it decodes correctly I guess? we can't know for sure, but hopefully it will
            xSemaphoreGive(validCamPacket);
        } else {
            // so the Teensy actually only sends one message to the ESP32 so we can ignore the message id
            fields = LSlaveToMaster_fields;
            dest = &lastLSlaveData;
        }

        if (xSemaphoreTake(uartDataSem, pdMS_TO_TICKS(SEMAPHORE_UNLOCK_TIMEOUT))){
            if (!pb_decode(&stream, fields, dest)){
                ESP_LOGW(TAG, "Failed to decode Protobuf on device %d, msg id %d, error: %s", device, msgType,
                            PB_GET_ERROR(&stream));
                vTaskDelay(pdMS_TO_TICKS(15));
            } else {
                // ESP_LOGI(TAG, "Successful decode of message id %d on device %d", msgType, device);
                esp_task_wdt_reset();
            }
            xSemaphoreGive(uartDataSem);
        } else {
            ESP_LOGW(TAG, "Failed to unlock UART semaphore, can't decode message on device id %d", device);
        }

        esp_task_wdt_reset();
    }
}

void comms_uart_init(uart_endpoint_t device){
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_port_t port = device == SBC_CAMERA ? UART_NUM_2 : UART_NUM_1;
    gpio_num_t txPin = device == SBC_CAMERA ? 16 : 19;
    gpio_num_t rxPin = device == SBC_CAMERA ? 17 : 18;
    ESP_LOGD(TAG, "UART endpoint is: %d, port is: %d, tx pin: %d, rx pin: %d", device, port, txPin, rxPin);

    // Configure UART parameters
    ESP_ERROR_CHECK(uart_param_config(port, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(port, txPin, rxPin, -1, -1));
    ESP_ERROR_CHECK(uart_driver_install(port, 256, 256, 8, NULL, 0));

    if (!createdSemaphore){
        uartDataSem = xSemaphoreCreateMutex();
        validCamPacket = xSemaphoreCreateBinary();

        // the main task will have to wait until a valid cam packet is received, so we lock the semaphore here
        xSemaphoreGive(validCamPacket);
        xSemaphoreTake(validCamPacket, portMAX_DELAY);
        xSemaphoreGive(uartDataSem);
        createdSemaphore = true;
        ESP_LOGD(TAG, "Created UART semaphores on first run");
    }

    char buf[64];
    snprintf(buf, 64, "JimBusDev%d", device);

    xTaskCreate(uart_receive_task, buf, 4096, (void*) device, configMAX_PRIORITIES - 1, NULL);
    ESP_LOGI(TAG, "UART init OK on device %d", device);
}

esp_err_t comms_uart_send(uart_endpoint_t device, msg_type_t msgId, uint8_t *pbData, size_t msgSize){
    if (msgSize > UINT8_MAX){
        ESP_LOGW(TAG, "Message too big to send properly, size is: %zu", msgSize);
    }

    char header[] = {0xB, msgId, msgSize};
    char end = 0xE;
    char checksum = (char) crc8(pbData, msgSize);
    uart_port_t port = device == SBC_CAMERA ? UART_NUM_2 : UART_NUM_1;

    uart_write_bytes(port, header, 3);
    uart_write_bytes(port, (char*) pbData, msgSize);
    uart_write_bytes(port, &checksum, 1);
    uart_write_bytes(port, &end, 1);

    // ESP_LOG_BUFFER_HEX_LEVEL(TAG, pbData, msgSize, ESP_LOG_INFO);
    // printf("DEVICE: %d, CHECKSUM: 0x%.2X\n", device, checksum);
    // printf("writing to device: %d, msg id: %d, %zu bytes of data\n", device, msgId, msgSize);

    return ESP_OK;
}

esp_err_t comms_uart_notify(uart_endpoint_t device, msg_type_t msgId){
    // not really implemented (as it's not needed as of now)
    ESP_LOGW(TAG, "comms_uart_notify() is not currently implemented");
    return ESP_FAIL;
}