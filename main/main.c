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
#include "bno055.h"

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

// Task which runs on the master. Receives sensor data from slave and handles complex routines
// like moving, finite state machines, Bluetooth, etc
static void master_task(void *pvParameter){
    static const char *TAG = "MasterTask";
    uint8_t robotId = 69;
    struct bno055_t bno055 = {0};
    s16 yawRaw = 0; // raw euler yaw data
    float yaw = 0.0f; // converted euler yaw data
    u8 sysCalib = 0;
    u8 magCalib = 0;
    u8 gyroCalib = 0;
    u8 accelCalib = 0;
    bno055.bus_read = bno055_read;
    bno055.bus_write = bno055_write;
    bno055.delay_msec = bno055_delay_ms;
    bno055.dev_addr = BNO055_I2C_ADDR2;

    print_reset_reason();

    // Initialise comms and hardware
    comms_i2c_init(I2C_NUM_0);
    comms_i2c_init(I2C_NUM_1);
    i2c_scanner(I2C_NUM_0);
    i2c_scanner(I2C_NUM_1);
    cam_init();

    s8 result = bno055_init(&bno055);
    result += bno055_set_power_mode(BNO055_POWER_MODE_NORMAL);
    // see page 22 of the datasheet, Section 3.3.1
    // we don't use NDOF or NDOF_FMC_OFF because it has a habit of snapping to magnetic north which is undesierable
    // instead we use IMUPLUS (acc + gyro fusion) if there is magnetic interference, otherwise M4G (basically relative mag)
    // edit this in defines.h
    result += bno055_set_operation_mode(BNO_MODE);
    if (result == 0){
        ESP_LOGI(TAG, "BNO055 init OK!");
    } else {
        ESP_LOGE(TAG, "BNO055 init error, current status: %d", result);
    }

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
        cam_calc();
        bno055_read_euler_h(&yawRaw);
        bno055_convert_float_euler_h_deg(&yaw);

        bno055_get_sys_calib_stat(&sysCalib);
        bno055_get_mag_calib_stat(&magCalib);
        bno055_get_accel_calib_stat(&accelCalib);
        bno055_get_gyro_calib_stat(&gyroCalib);
        // TODO we should use linear acceleration as well for robot velocity
        
        ESP_LOGI(TAG, "Euler heading: %f", yaw);
        ESP_LOGI(TAG, "Overall: %d, Mag: %d, Accel: %d, Gyro: %d", sysCalib, magCalib, accelCalib, gyroCalib);
        vTaskDelay(pdMS_TO_TICKS(100));
        goto end;

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
                robotState.inHeading = yaw;
                // robotState.inX = robotX;
                // robotState.inY = robotY;
                // TODO remove all these as all line stuff is done on the Teensy now
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
        
        msg.heading = yaw; // IMU heading
        msg.direction = 69.420f; // motor direction (which way we're driving)
        msg.orientation = 69.69f; // motor orientation (which way we're facing)
        msg.speed = 100.0f; // motor speed as 0-100%

        if (!pb_encode(&stream, I2CMasterProvide_fields, &msg)){
            ESP_LOGE(TAG, "I2C encode error: %s", PB_GET_ERROR(&stream));
        }
        printfln("Bytes written: %d", stream.bytes_written);

        comms_i2c_send(MSG_PUSH_I2C_MASTER, buf, stream.bytes_written);

        end:
        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(10)); // Random delay at of loop to allow motors to spin
    }
}

static void gpio_test_task(void *pvParameter){
    puts("Waiting...");
    vTaskDelay(pdMS_TO_TICKS(1000));
    puts("Running test...");
    gpio_set_direction(17, GPIO_MODE_OUTPUT);

    while (true){
        GPIO.out_w1ts = (1 << 17);
        GPIO.out_w1tc = (1 << 17);
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
    // xTaskCreatePinnedToCore(gpio_test_task, "GPIOTestTask", 8192, NULL, configMAX_PRIORITIES, NULL, APP_CPU_NUM);
}