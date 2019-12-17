#include "comms_i2c.h"
#include "HandmadeMath.h"
#include "states.h"
#include "pb_decode.h"
#include "soc/i2c_reg.h"
#include "soc/i2c_struct.h"

// i2c_data_t receivedData = {0};
nano_data_t nanoData = {0};

static const char *TAG = "CommsI2C";

/*
 * Teensy will be I2C slave, so we ask it for line data and other sensors, we process it, and then we give it back motors.
 * There are two actions we can do: pull and push data (request and provide, etc).
 * So the way this works is we begin a transaction to the slave with [0xB, MSG_PULL_SENSORDATA, 0, 0xEE]
 * All I2C runs on the main thread because it doesn't take much time and is simpler to do it that way.
 */

void comms_i2c_init(i2c_port_t port){
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = (port == I2C_NUM_0 ? 21 : 25),
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = (port == I2C_NUM_0 ? 22 : 26),
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        // 0.8 MHz, max is 1 MHz, unit is Hz
        // NOTE: 1MHz tends to break the i2c packets - use with caution!!
        .master.clk_speed = 900000,
    };
    ESP_ERROR_CHECK(i2c_param_config(port, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(port, conf.mode, 0, 0, 0));
    // workaround for clock stretching
    ESP_ERROR_CHECK(i2c_set_timeout(port, 0xFFFF));

    ESP_LOGI("CommsI2C_M", "I2C init OK on bus %d", port);
}

esp_err_t comms_i2c_send(msg_type_t msgId, uint8_t *pbData, size_t msgSize){
    uint8_t header[] = {0xB, msgId, msgSize};

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    ESP_ERROR_CHECK(i2c_master_start(cmd));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (I2C_SLAVE_DEV_ADDR << 1) | I2C_MASTER_WRITE, I2C_ACK_MODE));

    ESP_ERROR_CHECK(i2c_master_write(cmd, header, 3, I2C_ACK_MODE)); // write header
    ESP_ERROR_CHECK(i2c_master_write(cmd, pbData, msgSize, I2C_ACK_MODE)); // write buffer
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, 0xEE, I2C_ACK_MODE)); // write end byte

    ESP_ERROR_CHECK(i2c_master_stop(cmd));
    esp_err_t err = i2c_master_cmd_begin(I2C_SLAVE_DEV_BUS, cmd, pdMS_TO_TICKS(I2C_TIMEOUT));
    if (err != ESP_OK){
        ESP_LOGE(TAG, "Error in comms_i2c_send, message id = %d, size = %d: %s", msgId, msgSize,
                    esp_err_to_name(err));
        i2c_reset_tx_fifo(I2C_SLAVE_DEV_BUS);
        i2c_reset_rx_fifo(I2C_SLAVE_DEV_BUS);
        return err;
    }

    i2c_cmd_link_delete(cmd);
    return ESP_OK;
}

esp_err_t comms_i2c_workaround(msg_type_t msgId, uint8_t *pbData, size_t msgSize){
    uint8_t header[] = {0xB, msgId, msgSize};
    uint8_t recvSize = 2; // 16 bit int
    uint8_t rxBuffer[recvSize]; // bruh?
    
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (I2C_SLAVE_DEV_ADDR << 1), I2C_ACK_MODE);

    i2c_master_write(cmd, header, 3, I2C_ACK_MODE); // write header
    i2c_master_write(cmd, pbData, msgSize, I2C_ACK_MODE); // write bytes
    i2c_master_write_byte(cmd, 0xEE, I2C_ACK_MODE); // write end byte

    // Send repeated start
    i2c_master_start(cmd);
    // now send device address (indicating read) & read data
    i2c_master_write_byte(cmd, (I2C_SLAVE_DEV_ADDR << 1) | I2C_MASTER_READ, I2C_ACK_MODE);
    if (recvSize > 1) {
        i2c_master_read(cmd, rxBuffer, recvSize - 1, 0x0);
    }
    i2c_master_read_byte(cmd, rxBuffer + recvSize - 1, 0x1);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_SLAVE_DEV_BUS, cmd, portMAX_DELAY);
    i2c_cmd_link_delete(cmd);

    I2C_ERR_CHECK(ret);
    return ESP_OK;
}

esp_err_t comms_i2c_notify(msg_type_t msgId){
    uint8_t message[] = {0xB, msgId, 0, 0xEE};

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    ESP_ERROR_CHECK(i2c_master_start(cmd));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (I2C_SLAVE_DEV_ADDR << 1) | I2C_MASTER_WRITE, I2C_ACK_MODE));
    ESP_ERROR_CHECK(i2c_master_write(cmd, message, 4, I2C_ACK_MODE));
    ESP_ERROR_CHECK(i2c_master_stop(cmd));

    esp_err_t err = i2c_master_cmd_begin(I2C_SLAVE_DEV_BUS, cmd, pdMS_TO_TICKS(I2C_TIMEOUT));
    if (err != ESP_OK){
        ESP_LOGE(TAG, "Error in comms_i2c_notify: %s", esp_err_to_name(err));
        i2c_reset_tx_fifo(I2C_SLAVE_DEV_BUS);
        i2c_reset_rx_fifo(I2C_SLAVE_DEV_BUS);
        return err;
    }

    i2c_cmd_link_delete(cmd);
    return ESP_OK;
}