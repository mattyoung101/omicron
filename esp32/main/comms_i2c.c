/*
 * This file is part of the ESP32 firmware project.
 * Copyright (c) 2019-2020 Team Omicron. All rights reserved.
 *
 * Team Omicron members: Lachlan Ellis, Tynan Jones, Ethan Lo,
 * James Talkington, Matt Young.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "comms_i2c.h"
#include "HandmadeMath.h"
#include "states.h"
#include "pb_decode.h"
#include "soc/i2c_reg.h"
#include "soc/i2c_struct.h"

// i2c_data_t receivedData = {0};
nano_data_t nanoData = {0};
SemaphoreHandle_t nanoDataSem = NULL;
static const char *TAG = "CommsI2C";

static uint8_t nano_read(uint8_t addr, size_t size, uint8_t *data, robot_state_t *robotState) {
    uint8_t sendBytes[] = {0xB, robotState->outFRMotor + 100, robotState->outBRMotor + 100, -robotState->outBLMotor + 100, robotState->outFLMotor + 100, robotState->outResetMouse};
    uint8_t checksum = crc8(sendBytes, 6);

    // ESP_LOGI(TAG, "WRITING TO NANO");
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1), I2C_ACK_MODE);
    i2c_master_write(cmd, sendBytes, 6, I2C_ACK_MODE);
    // Send checksum boi
    i2c_master_write(cmd, &checksum, 1, I2C_ACK_MODE);
    // Send repeated start
    i2c_master_start(cmd);
    // now send device address (indicating read) & read data
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_READ, I2C_ACK_MODE);
    // ESP_LOGI(TAG, "READING FROM NANO");
    if (size > 1) {
        // ESP_LOGI(TAG, "SIZE > 1 APPARENTLY COOL");
        i2c_master_read(cmd, data, size - 1, 0x0);
    }
    i2c_master_read_byte(cmd, data + size - 1, 0x1);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, portMAX_DELAY);
    i2c_cmd_link_delete(cmd);

    I2C_ERR_CHECK(ret);
    return ESP_OK;
}

void comms_i2c_init_bno(i2c_port_t port){
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

    ESP_LOGI("CommsI2C_M", "BNO-055 I2C init OK on bus %d", port);
}

/** sends/receives data from the atmega slave **/
static void nano_comms_task(void *pvParameters){
    static const char *TAG = "JimBusLE";
    uint8_t buf[NANO_PACKET_SIZE] = {0};
    nanoDataSem = xSemaphoreCreateMutex();
    xSemaphoreGive(nanoDataSem);

    esp_task_wdt_add(NULL);
    ESP_LOGI(TAG, "Nano comms receive task init OK!");

    while (true){
        memset(buf, 0, NANO_PACKET_SIZE);
        nano_read(I2C_NANO_SLAVE_ADDR, NANO_PACKET_SIZE, buf, &robotState);

        if (buf[0] == I2C_BEGIN_DEFAULT){
            // ESP_LOGI(TAG, "FOUND START BYTE");
            if (xSemaphoreTake(nanoDataSem, pdMS_TO_TICKS(SEMAPHORE_UNLOCK_TIMEOUT))){
                nanoData.mouseDX = UNPACK_16(buf[1], buf[2]);
                nanoData.mouseDY = UNPACK_16(buf[3], buf[4]);
                xSemaphoreGive(nanoDataSem);
            } else {
                ESP_LOGE(TAG, "Failed to unlock nano data semaphore!");
            }
            // Read in and verify checksum, NOTE: IT DOES INCLUDE START BYTE COS THIS IS NOT PROTOBUF
            uint8_t receivedChecksum = buf[5];
            uint8_t actualChecksum = crc8(buf, 5);

            if(receivedChecksum != actualChecksum){
                ESP_LOGW(TAG, "Data integrity checked failed, expected 0x%.2X, got 0x%.2X", actualChecksum,
                        receivedChecksum);
                i2c_reset_rx_fifo(I2C_NUM_0); // Flush the buffer
                continue;
            } else {
                // Yay checksum verified, now check if reset flag has been received (not really)
                RS_SEM_LOCK
                if (robotState.outResetMouse) robotState.outResetMouse = false;
                RS_SEM_UNLOCK
            }
        } else {
            ESP_LOGE(TAG, "Invalid buffer, first byte is: 0x%X", buf[0]);
        }

        esp_task_wdt_reset();
    }
}

void comms_i2c_init_nano(i2c_port_t port){
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = 21,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = 22,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        // 0.25 MHz, max is 1 MHz, unit is Hz
        // NOTE: the ATMega328P can go up to 400 KHz
        .master.clk_speed = 100000,
    };
    ESP_ERROR_CHECK(i2c_param_config(port, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(port, conf.mode, 0, 0, 0));
    // TODO should probably remove this timeout workaround
    ESP_ERROR_CHECK(i2c_set_timeout(I2C_NUM_0, 0xFFFF));

    xTaskCreate(nano_comms_task, "JimBusLE", 4096, NULL, configMAX_PRIORITIES - 1, NULL);
    ESP_LOGI("CommsI2C_M", "ATMega I2C init OK as master (RL slave) on bus %d", port);
}

esp_err_t comms_i2c_send(msg_type_t msgId, uint8_t *pbData, size_t msgSize){
    // uint8_t header[] = {0xB, msgId, msgSize};

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    ESP_ERROR_CHECK(i2c_master_start(cmd));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (I2C_SLAVE_DEV_ADDR << 1) | I2C_MASTER_WRITE, I2C_ACK_MODE));

    // ESP_ERROR_CHECK(i2c_master_write(cmd, header, 3, I2C_ACK_MODE)); // write header
    ESP_ERROR_CHECK(i2c_master_write(cmd, pbData, msgSize, I2C_ACK_MODE)); // write buffer
    // ESP_ERROR_CHECK(i2c_master_write_byte(cmd, 0xEE, I2C_ACK_MODE)); // write end byte

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