#define HANDMADE_MATH_IMPLEMENTATION
#define HANDMADE_MATH_NO_SSE
#define _GNU_SOURCE
#include "HandmadeMath.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "defines.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "nvs_flash.h"
#include <math.h>
#include "motor.h"
#include <str.h>
#include "fsm.h"
#include "cam.h"
#include "states.h"
#include "soc/efuse_reg.h"
#include "comms_i2c.h"
#include "esp_timer.h"
#include "esp_task_wdt.h"
#include "simple_imu.h"
#include "pid.h"
#include "vl53l0x_api.h"
#include "comms_bluetooth.h"
#include "pb_encode.h"
#include "pb_decode.h"
#include "i2c.pb.h"
#include "lrf.h"
#include "driver/spi_master.h"

#if ENEMY_GOAL == GOAL_YELLOW
    #define AWAY_GOAL goalYellow
    #define HOME_GOAL goalBlue
#elif ENEMY_GOAL == GOAL_BLUE
    #define AWAY_GOAL goalBlue
    #define HOME_GOAL goalYellow
#endif

state_machine_t *stateMachine = NULL;
static const char *RST_TAG = "ResetReason";

static void print_reset_reason(){
    esp_reset_reason_t resetReason = esp_reset_reason();
    if (resetReason == ESP_RST_PANIC){
        ESP_LOGW(RST_TAG, "Reset due to panic!");
    } else if (resetReason == ESP_RST_INT_WDT || resetReason == ESP_RST_TASK_WDT || resetReason == ESP_RST_WDT){
        ESP_LOGW(RST_TAG, "Reset due to watchdog timer!");
    } else {
        ESP_LOGD(RST_TAG, "Other reset reason: %d", resetReason);
    }
}

// Task which runs on the master. Receives sensor data from slave and handles complex routines
// like moving, finite state machines, Bluetooth, etc
static void master_task(void *pvParameter){
    static const char *TAG = "MasterTask";
    uint8_t robotId = 69;

    print_reset_reason();

    // Initialise comms and hardware
    comms_i2c_init(I2C_NUM_0);
    cam_init();
    gpio_set_direction(KICKER_PIN, GPIO_MODE_OUTPUT);
    ESP_LOGI(TAG, "=============== Master hardware init OK ===============");

    // read robot ID from NVS and init Bluetooth
    nvs_get_u8_graceful("RobotSettings", "RobotID", &robotId);
    defines_init(robotId);
    ESP_LOGI(TAG, "Running as robot #%d", robotId);
    robotState.inRobotId = robotId;

    #ifdef BLUETOOTH_ENABLED
    if (robotId == 0){
        comms_bt_init_master();
    } else {
        comms_bt_init_slave();
    }
    #endif

    // Initialise FSM, start out in defence until we get a BT connection
    #if DEFENCE
    stateMachine = fsm_new(&stateDefenceDefend);
    #else
    stateMachine = fsm_new(&stateAttackPursue);
    #endif

    // Wait for the slave to calibrate IMU and send over the first packets
    ESP_LOGI(TAG, "Waiting for slave IMU calibration to complete...");
    vTaskDelay(pdMS_TO_TICKS(IMU_CALIBRATION_COUNT * IMU_CALIBRATION_TIME + 1000));
    ESP_LOGI(TAG, "Running!");

    esp_task_wdt_add(NULL);

    while (true){
        // update sensors
        cam_calc();
        comms_i2c_send_receive(MSG_PULL_I2C_SLAVE, NULL, 0); // TODO actually encode what we're sending properly

        // update values for FSM, mutexes are used to prevent race conditions
        // TODO we need much less of these semaphores because it runs on the main thread
        if (xSemaphoreTake(robotStateSem, pdMS_TO_TICKS(SEMAPHORE_UNLOCK_TIMEOUT)) && 
            xSemaphoreTake(goalDataSem, pdMS_TO_TICKS(SEMAPHORE_UNLOCK_TIMEOUT))){
                // reset out values
                robotState.outShouldBrake = false;
                robotState.outOrientation = 0;
                robotState.outDirection = 0;
                robotState.outSwitchOk = false;

                // update FSM values
                robotState.inBallAngle = orangeBall.angle;
                robotState.inBallStrength = orangeBall.length;
                // TODO make goal stuff floats as well
                if (robotState.outIsAttack){
                    robotState.inGoalVisible = AWAY_GOAL.exists;
                    robotState.inGoalAngle = AWAY_GOAL.angle + CAM_ANGLE_OFFSET;
                    robotState.inGoalLength = (int16_t) AWAY_GOAL.length;
                    robotState.inGoalDistance = AWAY_GOAL.distance;

                    robotState.inOtherGoalVisible = HOME_GOAL.exists;
                    robotState.inOtherGoalAngle = HOME_GOAL.angle + CAM_ANGLE_OFFSET;
                    robotState.inOtherGoalLength = (int16_t) HOME_GOAL.length;
                    robotState.inOtherGoalDistance = HOME_GOAL.distance;
                } else {
                    robotState.inOtherGoalVisible = AWAY_GOAL.exists;
                    robotState.inOtherGoalAngle = AWAY_GOAL.angle + CAM_ANGLE_OFFSET;
                    robotState.inOtherGoalLength = (int16_t) AWAY_GOAL.length;
                    robotState.inOtherGoalDistance = AWAY_GOAL.distance;

                    robotState.inGoalVisible = HOME_GOAL.exists;
                    robotState.inGoalAngle = HOME_GOAL.angle + CAM_ANGLE_OFFSET;
                    robotState.inGoalLength = (int16_t) HOME_GOAL.length;
                    robotState.inGoalDistance = HOME_GOAL.distance;
                }
                robotState.inHeading = lastSensorUpdate.heading;
                // robotState.inX = robotX;
                // robotState.inY = robotY;
                robotState.inBatteryVoltage = lastSensorUpdate.voltage;
                robotState.inLineAngle = lastSensorUpdate.lineAngle;
                robotState.inLineSize = lastSensorUpdate.lineSize;
                robotState.inLastAngle = lastSensorUpdate.lastAngle;
                robotState.inOnLine = lastSensorUpdate.onLine;
                robotState.inLineOver = lastSensorUpdate.lineOver;

                // unlock semaphores
                xSemaphoreGive(robotStateSem);
                xSemaphoreGive(goalDataSem);
        } else {
            ESP_LOGW(TAG, "Failed to acquire semaphores, cannot update FSM data.");
        }

        // update the actual FSM
        fsm_update(stateMachine);

        // robotState.outSpeed = 0;
        // imu_correction(&robotState);

        // line over runs after the FSM to override it
        update_line(&robotState);

        // print_ball_data(&robotState);
        // print_goal_data(&robotState);
        // vTaskDelay(pdMS_TO_TICKS(250));
        // print_motion_data(&robotState);
        // print_position_data(&robotState);
        // printf("%f\n", robotState.inLineAngle);

        motor_calc(robotState.outDirection, robotState.outOrientation, robotState.outSpeed);
        
        // transmit motion data back to the slave
        // TODO actually do it with Protobuf
        comms_i2c_send(MSG_PUSH_I2C_MASTER, NULL, 0);

        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(10)); // Random delay at of loop to allow motors to spin
    }
}

void motor_test_task(void *pvParameter){
    static const char *TAG = "TestTask";
    uint8_t dataIn[10] = {0};
    uint8_t dataOut[] = {'E', 'S', 'P', '2', 'T', 'E', 'E', 'N', 'S', 'Y'};

    spi_bus_config_t conf = {
        .mosi_io_num = 13,
        .miso_io_num = 12,
        .sclk_io_num = 14,
        .max_transfer_sz = 256,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(HSPI_HOST, &conf, 1));

    spi_device_handle_t teensyHandle = NULL;
    spi_device_interface_config_t teensyConf = {
        .mode = 0,
        .clock_speed_hz = SPI_MASTER_FREQ_8M,
        .queue_size = 3,
        .spics_io_num = 15, // TODO make this a define (chip select pin)
        .duty_cycle_pos = 128, // 50% duty cycle
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0,
        .cs_ena_posttrans = 3
    };
    ESP_ERROR_CHECK(spi_bus_add_device(HSPI_HOST, &teensyConf, &teensyHandle));

    while (true){
        memset(dataIn, 0, 10);

        spi_transaction_t trans = {0};
        trans.length = 10 * 8;
        trans.tx_buffer = dataOut;
        trans.rx_buffer = dataIn;
        ESP_ERROR_CHECK(spi_device_transmit(teensyHandle, &trans));

        size_t len = trans.rxlength / 8;
        ESP_LOGI(TAG, "Transmission completed, received %d bytes", len);
        ESP_LOG_BUFFER_HEXDUMP(TAG, trans.rx_buffer, len, ESP_LOG_INFO);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void app_main(){
    puts("====================================================================================");
    puts(" * This ESP32 belongs to a robot from Team Omicron at Brisbane Boys' College.");
    puts(" * Software copyright (c) 2019 Team Omicron. All rights reserved.");
    puts("====================================================================================");

    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGI("AppMain", "Reflashing NVS");
        // NVS partition was truncated and needs to be erased
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    // AutoMode: automatically assign to slave/master depending on a value set in NVS
    nvs_handle storageHandle;
    ESP_ERROR_CHECK(nvs_open("RobotSettings", NVS_READWRITE, &storageHandle));

    // write master/slave/robot ID to NVS if configured
    #if defined NVS_WRITE_ROBOTNUM
        ESP_ERROR_CHECK(nvs_set_u8(storageHandle, "RobotID", NVS_WRITE_ROBOTNUM));
        ESP_LOGE("RobotID", "Successfully wrote robot number to NVS.");
    #endif

    #if defined NVS_WRITE_ROBOTNUM
        ESP_ERROR_CHECK(nvs_commit(storageHandle));
    #endif

    nvs_close(storageHandle);
    fflush(stdout);

    // create the main (or test, uncomment it if you want that) task 
    xTaskCreatePinnedToCore(master_task, "MasterTask", 12048, NULL, configMAX_PRIORITIES, NULL, APP_CPU_NUM);
    // xTaskCreate(motor_test_task, "MotorTestTask", 8192, NULL, configMAX_PRIORITIES, NULL);
}