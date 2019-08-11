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
#include "pid.h"
#include "vl53l0x_api.h"
#include "comms_bluetooth.h"
#include "pb_encode.h"
#include "pb_decode.h"
#include "i2c.pb.h"
#include "lrf.h"
#include "driver/spi_master.h"
#include "sh2.h"
#include "sh2_err.h"
#include "sh2_SensorValue.h"
#include "sh2_hal.h"

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
        // ESP_LOGD(RST_TAG, "Other reset reason: %d", resetReason);
    }
}

// BNO-080 event handlers
static QueueHandle_t bnoEvtQueue = NULL;
static void bno_event_handler(void *cookie, sh2_AsyncEvent_t *pEvent){
    printf("bno_event_handler id: %d\n", pEvent->eventId);
    if (pEvent->eventId == SH2_RESET){
        ESP_LOGI("BNO080", "BNO080 reset performed");
    }
}
static void bno_sensor_handler(void *cookie, sh2_SensorEvent_t *pEvent){
    printf("bno_sensor_handler report id: %d\n", pEvent->reportId);
    xQueueSend(bnoEvtQueue, pEvent, pdMS_TO_TICKS(250));
}

// Task which runs on the master. Receives sensor data from slave and handles complex routines
// like moving, finite state machines, Bluetooth, etc
static void master_task(void *pvParameter){
    static const char *TAG = "MasterTask";
    uint8_t robotId = 69;

    print_reset_reason();
    bnoEvtQueue = xQueueCreate(8, sizeof(sh2_SensorEvent_t));

    // Initialise comms and hardware
    comms_i2c_init(I2C_NUM_0);
    comms_i2c_init(I2C_NUM_1);
    cam_init();
    gpio_set_direction(BNO_RSTN_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(BNO_INTN_PIN, GPIO_MODE_INPUT);

    ESP_LOGI(TAG, "Waiting..."); // (for debug so the sensor doesn't init)
    vTaskDelay(pdMS_TO_TICKS(5000));
    ESP_LOGI(TAG, "Running!");

    // Initialise BNO
    sh2_hal_init();
    sh2_initialize(bno_event_handler, NULL);
    sh2_setSensorCallback(bno_sensor_handler, NULL);
    vTaskDelay(pdMS_TO_TICKS(250)); // wait for init

    // enable dynamic planar calibration (apparently used on robotic vacuum cleaners which is similar to us)
    sh2_setCalConfig(SH2_CAL_PLANAR);

    // Initialise the rotation vector sensor on BNO
    sh2_SensorConfig_t conf = {0};
    conf.alwaysOnEnabled = true;
    conf.reportInterval_us = 10000;
    int err = sh2_setSensorConfig(SH2_ROTATION_VECTOR, &conf);
    printf("Set config of rotation vec, error id %d\n", err);

    // print the product IDs of the sensor (for testing)
    sh2_ProductIds_t prodIds = {0};
    printf("Prod id return code: %d\n", sh2_getProdIds(&prodIds));
    puts("===== BNO080 Product IDs =====");
    for (int n = 0; n < prodIds.numEntries; n++) {
        printf("Part %d : Version %d.%d.%d Build %d\n",
               prodIds.entry[n].swPartNumber,
               prodIds.entry[n].swVersionMajor, prodIds.entry[n].swVersionMinor, 
               prodIds.entry[n].swVersionPatch, prodIds.entry[n].swBuildNumber);
    }

    i2c_scanner(I2C_NUM_0);
    i2c_scanner(I2C_NUM_1);

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

    esp_task_wdt_add(NULL);

    while (true){
        // update sensors
        // cam_calc();
        // TODO check BNO event queue

        TASK_HALT; // FIXME remove this after BNO init testing

        // update values for FSM, mutexes are used to prevent race conditions
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

        motor_calc(robotState.outDirection, robotState.outOrientation, robotState.outSpeed);
        
        // encode and send Protobuf message to Teenys slave
        I2CMasterProvide msg = I2CMasterProvide_init_default;
        uint8_t buf[PROTOBUF_SIZE] = {0};
        pb_ostream_t stream = pb_ostream_from_buffer(buf, PROTOBUF_SIZE);
        
        msg.direction = 69.420f;
        msg.heading = 123.456f;
        msg.orientation = 69.69f;
        msg.speed = 100.0f;

        if (!pb_encode(&stream, I2CMasterProvide_fields, &msg)){
            ESP_LOGE(TAG, "I2C encode error: %s", PB_GET_ERROR(&stream));
        }
        printfln("Bytes written: %d", stream.bytes_written);

        comms_i2c_send(MSG_PUSH_I2C_MASTER, buf, stream.bytes_written);

        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(10)); // Random delay at of loop to allow motors to spin
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