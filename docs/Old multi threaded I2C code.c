// TODO put this into the send receive function
static void comms_i2c_receive_task(void *pvParameters){
    static const char *TAG = "I2CReceiveTask";
    uint8_t buf[PROTOBUF_SIZE] = {0};
    uint8_t msg[PROTOBUF_SIZE] = {0};
    uint16_t goodPackets = 0;
    pbSem = xSemaphoreCreateMutex();
    xSemaphoreGive(pbSem);

    // esp_task_wdt_add(NULL);
    ESP_LOGI(TAG, "Slave I2C task init OK");

    while (true){
        uint8_t byte = 0;
        uint8_t i = 0;
        memset(buf, 0, PROTOBUF_SIZE);
        memset(msg, 0, PROTOBUF_SIZE);

        // attempt to read in bytes one by one
        while (true){
            i2c_slave_read_buffer(I2C_NUM_0, &byte, 1, portMAX_DELAY);
            buf[i++] = byte;

            // if we've got the end byte (0xEE) then quit
            if (byte == 0xEE){
                break;
            }
        }

        if (buf[0] == 0xB){
            msg_type_t msgId = buf[1];
            uint8_t msgSize = buf[2];

            // remove the header by copying from byte 3 onwards, excluding the end byte (0xEE)
            memcpy(msg, buf + 3, msgSize);

            pb_istream_t stream = pb_istream_from_buffer(msg, msgSize);
            void *dest = NULL;
            void *msgFields = NULL;

            // assign destination struct based on message ID
            switch (msgId){
                case MSG_PULL_I2C_SLAVE: // FIXME its different now
                    dest = (void*) &lastSensorUpdate;
                    msgFields = (void*) &SensorUpdate_fields;
                    break;
                default:
                    ESP_LOGW(TAG, "Unknown message ID: %d", msgId);
                    continue;
            }

            SensorUpdate oldUpdate = lastSensorUpdate;

            // semaphore required since we use the protobuf messages outside this thread
            if (xSemaphoreTake(pbSem, pdMS_TO_TICKS(SEMAPHORE_UNLOCK_TIMEOUT))){
                if (!pb_decode(&stream, msgFields, dest)){
                    ESP_LOGE(TAG, "Protobuf decode error for message ID %d: %s", msgId, PB_GET_ERROR(&stream));
                } else {
                    // to save the values from being 0, if heading or TSOP looks wrong, reject the message and restore
                    // the last one
                    // NOTE: this solution is far from ideal, but I'm in a rush and am unable to find whereabouts
                    // or why the valid is being set to zero
                    if (lastSensorUpdate.heading <= 0.01f && 
                        (lastSensorUpdate.tsopStrength <= 0.01f || lastSensorUpdate.tsopAngle <= 0.01f)){
                        // ESP_LOGW(TAG, "Rejecting invalid message, restoring last message");
                        lastSensorUpdate = oldUpdate;
                    } else {
                        goodPackets++;
                    }
                }
                
                xSemaphoreGive(pbSem);
            } else {
                ESP_LOGE(TAG, "Failed to unlock Protobuf semaphore!");
            }
        } else {
            // ESP_LOGW(TAG, "Invalid buffer, first byte is: 0x%X, previous good packets: %d", buf[0], goodPackets);
            goodPackets = 0;

            // reset I2C and try to correct the issue by waiting
            i2c_reset_rx_fifo(I2C_NUM_0);
            vTaskDelay(pdMS_TO_TICKS(15));
        }

        esp_task_wdt_reset();
    }
}