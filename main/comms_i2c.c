#include "comms_i2c.h"
#include "HandmadeMath.h"
#include "states.h"
#include "pb_decode.h"
#include "soc/i2c_reg.h"
#include "soc/i2c_struct.h"

i2c_data_t receivedData = {0};
nano_data_t nanoData = {0};
SensorUpdate lastSensorUpdate = SensorUpdate_init_zero;
SemaphoreHandle_t pbSem = NULL;

static const char *TAG = "CommsI2C";

// TODO packet timeout if we haven't received a packet on the master in x ms

/*
 * Teensy will be I2C slave, so we ask it for line data and other sensors, we process it, and then we give it back motors.
 * There are two actions we can do: pull and push data (request and provide, etc).
 * So the way this works is we begin a transaction to the slave with [0xB, MSG_PULL_SENSORDATA, 0, 0xEE]
 * All I2C runs on the main thread because it doesn't take much time and is simpler to do it that way.
 */

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

void comms_i2c_init(i2c_port_t port){
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = 21,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = 22,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        // 0.8 MHz, max is 1 MHz, unit is Hz
        // NOTE: 1MHz tends to break the i2c packets - use with caution!!
        .master.clk_speed = 800000,
    };
    ESP_ERROR_CHECK(i2c_param_config(port, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(port, conf.mode, 0, 0, 0));
    // Nano keeps timing out, so fuck it, let's yeet the timeout value. default value is 1600, max is 0xFFFFF
    // TODO do we still need this hack given that we don't have a Nano?
    ESP_ERROR_CHECK(i2c_set_timeout(I2C_NUM_0, 0xFFFF));
    
    ESP_LOGI("CommsI2C_M", "I2C init OK on bus %d", port);
}

esp_err_t comms_i2c_send_receive(msg_type_t msgId, uint8_t *pbData, size_t msgSize){
    uint8_t rxBuf[PROTOBUF_SIZE] = {0}; // buffer for data pulled out of I2C
    uint8_t buf[PROTOBUF_SIZE] = {0}; // buffer that acts as a working space for Protobuf decoding

    uint8_t header[] = {0xB, msgId, msgSize};
    uint8_t finish = 0xEE;

    // first, transmit the message
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (I2C_SLAVE_DEV_ADDR << 1), I2C_ACK_MODE);
    
    i2c_master_write(cmd, header, 3, I2C_ACK_MODE); // write header
    i2c_master_write(cmd, pbData, msgSize, I2C_ACK_MODE); // write buffer
    i2c_master_write(cmd, &finish, 1, I2C_ACK_MODE); // write end byte
    
    // now read the message coming from the slave
    // send repeated start
    i2c_master_start(cmd);
    // now send device address (indicating read) & read data
    i2c_master_write_byte(cmd, (I2C_SLAVE_DEV_ADDR << 1) | I2C_MASTER_READ, I2C_ACK_MODE);

    // TODO I think we need to read these in one byte at a time somehow????
    if (msgSize > 1) {
        // i2c_master_read(cmd, rxBuf, size - 1, 0x0);
    }
    // i2c_master_read_byte(cmd, data + size - 1, 0x1);
    
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(I2C_TIMEOUT));
    i2c_cmd_link_delete(cmd);

    I2C_ERR_CHECK(ret);
    return ESP_OK;

    // blah blah blah
    return ESP_OK;
}

esp_err_t comms_i2c_send(msg_type_t msgId, uint8_t *pbData, size_t msgSize){
    uint8_t header[] = {0xB, msgId, msgSize};
    uint8_t finish = 0xEE;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    ESP_ERROR_CHECK(i2c_master_start(cmd));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (I2C_SLAVE_DEV_ADDR << 1) | I2C_MASTER_WRITE, I2C_ACK_MODE));

    ESP_ERROR_CHECK(i2c_master_write(cmd, header, 3, I2C_ACK_MODE)); // write header
    ESP_ERROR_CHECK(i2c_master_write(cmd, pbData, msgSize, I2C_ACK_MODE)); // write buffer
    ESP_ERROR_CHECK(i2c_master_write(cmd, &finish, 1, I2C_ACK_MODE)); // write end byte

    ESP_ERROR_CHECK(i2c_master_stop(cmd));
    esp_err_t err = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(I2C_TIMEOUT));
    if (err != ESP_OK){
        ESP_LOGE(TAG, "Error in comms_i2c_send writing message id %d of size %d: %s", msgId, msgSize,
                    esp_err_to_name(err));
        i2c_reset_tx_fifo(I2C_NUM_0);
        i2c_reset_rx_fifo(I2C_NUM_0);
        return err;
    }

    i2c_cmd_link_delete(cmd);
    return ESP_OK;
}

esp_err_t comms_i2c_notify(msg_type_t msgId){
    uint8_t message[] = {0xB, msgId, 0, 0xEE};

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    ESP_ERROR_CHECK(i2c_master_start(cmd));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (I2C_SLAVE_DEV_ADDR << 1) | I2C_MASTER_WRITE, I2C_ACK_MODE));
    ESP_ERROR_CHECK(i2c_master_write(cmd, message, 4, I2C_ACK_MODE));
    ESP_ERROR_CHECK(i2c_master_stop(cmd));

    esp_err_t err = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(I2C_TIMEOUT));
    if (err != ESP_OK){
        ESP_LOGE(TAG, "Error in comms_i2c_notify: %s", esp_err_to_name(err));
        i2c_reset_tx_fifo(I2C_NUM_0);
        i2c_reset_rx_fifo(I2C_NUM_0);
        return err;
    }

    i2c_cmd_link_delete(cmd);
    return ESP_OK;
}