#include "comms_uart.h"

static const char *TAG = "CommsUART";
ObjectData lastObjectData = ObjectData_init_zero;
LocalisationData lastLocaliserData = LocalisationData_init_zero;
ESP32DebugCommand lastDebugCmd = ESP32DebugCommand_init_zero;
LSlaveToMaster lastLSlaveData = LSlaveToMaster_init_zero;

SemaphoreHandle_t uartDataSem = NULL;
SemaphoreHandle_t validCamPacket = NULL;
static bool createdSemaphore = false;

static void uart_receive_task(void *pvParameter){
    static const char *TAG = "UARTReceiveTask";
    
    esp_task_wdt_add(NULL);
    uart_endpoint_t device = (uart_endpoint_t) pvParameter;
    uart_port_t port = device == SBC_CAMERA ? UART_NUM_2 : UART_NUM_1;
    ESP_LOGI(TAG, "UART receive task init OK on endpoint: %d", device);

    while (true){
        // first let's read in the header and see what message id, and how much we need to read in
        // message format is: [0xB, msg_type, size, ...data..., 0xE] - so we have a 3 byte header
        uint8_t header[3] = {0};
        uart_read_bytes(port, header, 3, portMAX_DELAY);
        esp_task_wdt_reset();

        if (header[0] == 0xB){
            msg_type_t msgType = header[1];
            uint8_t msgSize = header[2];
//            ESP_LOGD(TAG, "received an OK message on device: %d, msgType: %d, msgSize: %d", device, msgType, msgSize);

            // read in the rest of the data
            uint8_t data[msgSize];
            uart_read_bytes(port, data, msgSize, pdMS_TO_TICKS(250));
            // TODO it's possible that we may also have to read in the buffer one by one in case the UART FIFO is full?
            // like we did with the old comms_i2c.c in the 2019 deus vult repo

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
//                    ESP_LOGI(TAG, "Wow we actually decoded a message on device: %d", device);
                }
                xSemaphoreGive(uartDataSem);
            } else {
                ESP_LOGW(TAG, "Failed to unlock UART semaphore, can't decode message on device id %d", device);
            }

            esp_task_wdt_reset();
            uart_flush_input(port);
        } else {
            ESP_LOGW(TAG, "Received invalid UART packet, begin byte was 0x%.2X not 0x0B, device: %d", header[0], device);
            uart_flush_input(port);
            esp_task_wdt_reset();
            vTaskDelay(pdMS_TO_TICKS(15));
        }

        // memset(buffer, 0, 5);
        // esp_task_wdt_reset();
        // uart_read_bytes(UART_NUM_1, buffer, 5, pdMS_TO_TICKS(4096));
        // // ESP_LOGI(TAG, "BULLSHIT");

        // if (buffer[0] == 0xB && buffer[1] == 0xB) {
        //     ESP_LOGW(TAG, "Found start byte");
        //     if (xSemaphoreTake(robotStateSem, pdMS_TO_TICKS(SEMAPHORE_UNLOCK_TIMEOUT))) {
        //         robotState.inLineAngle = (buffer[2] << 8) | (buffer[3] & 0xFF);
        //         robotState.inLineSize = buffer[4] / 100;
        //         xSemaphoreGive(robotStateSem);
        //     } else {
        //         ESP_LOGW(TAG, "Unable to acquire semaphore in time!");
        //     }
        // }

        // uart_flush_input(UART_NUM_1);
        // esp_task_wdt_reset();
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
    gpio_num_t txPin = device == SBC_CAMERA ? 17 : 19;
    gpio_num_t rxPin = device == SBC_CAMERA ? 16 : 18;
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
    snprintf(buf, 64, "UARTRecvTask%d", device);

    xTaskCreate(uart_receive_task, buf, 4096, (void*) device, configMAX_PRIORITIES - 1, NULL);
    ESP_LOGI(TAG, "UART comms init OK!");
}

esp_err_t comms_uart_send(uart_endpoint_t device, msg_type_t msgId, uint8_t *pbData, size_t msgSize){
    if (msgSize > UINT8_MAX){
        ESP_LOGW(TAG, "Message too big to send properly, size is: %zu", msgSize);
    }

    char header[] = {0xB, msgId, msgSize};
    char end = 0xE;
    uart_port_t port = device == SBC_CAMERA ? UART_NUM_2 : UART_NUM_1;

    uart_write_bytes(port, header, 3);
    uart_write_bytes(port, (char*) pbData, msgSize);
    uart_write_bytes(port, &end, 1);

//    printf("writing to device: %d, msg id: %d, %zu bytes of data\n", device, msgId, msgSize);

    return ESP_OK;
}

esp_err_t comms_uart_notify(uart_endpoint_t device, msg_type_t msgId){
    // not really implemented (as it's not needed as of now)
    ESP_LOGW(TAG, "comms_uart_notify() is not currently implemented");
    return ESP_FAIL;
}