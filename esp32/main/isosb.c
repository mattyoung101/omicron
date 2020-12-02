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
#include "isosb.h"
#include "driver/i2c.h"
#include "defines.h"
#include "alloca.h"
#include "utils.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "string.h"
#include "esp_task_wdt.h"
#include "driver/uart.h"
#include "bno055.h"
#include "comms_i2c.h"

// ISOSB Test Suite - "I'm Sick of Shit Breaking"
// Tries to test every peripheral (for example, BNO-055, Nano comms, etc) to its full capacity and reports any failures
// To run ISOSB comment in the code which runs the isosb task in main.c, and comment out master_task
// Feel free to use the enterprise friendly name: Integrated Sub-tests Ordered Similarly By (name)

#define TEST_SUCCESS do { ESP_LOGI(TAG, "----> TEST OK!"); } while (0);
#define TEST_FAIL do { ESP_LOGE(TAG, "-----> TEST FAILED!"); vTaskSuspend(NULL); } while (true);
#define TEST_WAIT_KEY do { uint8_t byte = 0; uart_read_bytes(UART_NUM_0, &byte, 1, portMAX_DELAY); } while (0);
static const char *TAG = "ISOSB";

static bool i2c_check(const char *name, uint8_t address, i2c_port_t port){
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, 1 /* expect ack */);
    i2c_master_stop(cmd);

    esp_err_t response = i2c_master_cmd_begin(port, cmd, 75 / portTICK_PERIOD_MS);
    if (response == ESP_OK){
        return true;
    } else {
        ESP_LOGW(TAG, "I2C device %s on 0x%.2X (port %d) does not exist (response: %s)", name, address, port, esp_err_to_name(response));
        return false;
    }
    i2c_cmd_link_delete(cmd);
}

static bool test_bno_init(struct bno055_t *bno055){
    u16 swRevId = 0;
    u8 chipId = 0;

    bno055->bus_read = bno055_read;
    bno055->bus_write = bno055_write;
    bno055->delay_msec = bno055_delay_ms;
    bno055->dev_addr = BNO055_I2C_ADDR2;
    s8 result = bno055_init(bno055);
    result += bno055_set_power_mode(BNO055_POWER_MODE_NORMAL);
    // see page 22 of the datasheet, Section 3.3.1
    // we don't use NDOF or NDOF_FMC_OFF because it has a habit of snapping to magnetic north which is undesierable
    // instead we use IMUPLUS (acc + gyro fusion) if there is magnetic interference, otherwise M4G (basically relative mag)
    result += bno055_set_operation_mode(BNO055_OPERATION_MODE_IMUPLUS);
    result += bno055_read_sw_rev_id(&swRevId);
    result += bno055_read_chip_id(&chipId);
    if (result == 0){
        return true;
    } else {
        ESP_LOGE("BNO055_HAL", "BNO055 init error, current status: %d", result);
        return false;
    }
}

/** ISOSB test task - "I'm Sick of Shit Breaking" - used to test peripherals on an ESP32 */
void isosb_test_task(void *pvParameter){
    ESP_LOGI(TAG, "Welcome to I'm Sick of Shit Breaking (ISOSB), the Omicron ESP32 testing tool. Runing tests...");

    ESP_LOGD(TAG, "TEST 1: The ESP32 has powered on and FreeRTOS is running...");
    TEST_SUCCESS;

    ESP_LOGD(TAG, "push a damn key");
    TEST_WAIT_KEY;
    puts("well we read soething");
    
    ESP_LOGD(TAG, "TEST 2: BNO-055 is connected...");
    comms_i2c_init_bno(I2C_NUM_1); // TODO check this as well
    if (i2c_check("BNO-055", BNO055_I2C_ADDR2, I2C_NUM_1)){
        TEST_SUCCESS;
    } else {
        TEST_FAIL;
    }

    ESP_LOGD(TAG, "TEST 2: BNO-055 inits successfully...");
    struct bno055_t bno055 = {0};
    if (test_bno_init(&bno055)){
        TEST_SUCCESS;
    } else {
        TEST_FAIL;
    }

    ESP_LOGD(TAG, "TEST 3: BNO-055 responds to input. Please ROTATE the robot then PRESS enter key...");
    float oldYaw;
    if (bno055_convert_float_euler_h_deg(&oldYaw) != BNO055_SUCCESS){
        TEST_FAIL;
    } else {
        TEST_WAIT_KEY;
        float newYaw;
        if (bno055_convert_float_euler_h_deg(&newYaw)!= BNO055_SUCCESS){
            TEST_FAIL;
        } else if (newYaw == oldYaw){
            ESP_LOGE(TAG, "newYaw == oldYaw (%f == %f)", newYaw, oldYaw);
            TEST_FAIL;
        }
        TEST_SUCCESS;
    }

    ESP_LOGD(TAG, "TEST 4: ATMega is connected...");
    comms_i2c_init_nano(I2C_NUM_0); // TODO check this
    if (i2c_check("ATMega", I2C_NANO_SLAVE_ADDR, I2C_NUM_0)){
        TEST_SUCCESS;
    } else {
        TEST_FAIL;
    }

    ESP_LOGI(TAG, "=============== ISOSB TESTS PASSED SUCCESSFULLY ===============");
    ESP_LOGI(TAG, "Congratulations, looks like the robot works. Nothing left to do.");
    vTaskSuspend(NULL);
    while (true);
}