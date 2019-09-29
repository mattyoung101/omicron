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
#include <string.h>
#include "fsm.h"
#include "cam.h"
#include "states.h"
#include "comms_i2c.h"
#include "comms_uart.h"
#include "esp_timer.h"
#include "esp_task_wdt.h"
#include "pid.h"
#include "comms_bluetooth.h"
#include "pb_encode.h"
#include "pb_decode.h"
#include "i2c.pb.h"
#include "bno055.h"
#include "button.h"
#include "movavg.h"

#if ENEMY_GOAL == GOAL_YELLOW
    #define AWAY_GOAL goalYellow
    #define HOME_GOAL goalBlue
#elif ENEMY_GOAL == GOAL_BLUE
    #define AWAY_GOAL goalBlue
    #define HOME_GOAL goalYellow
#endif

static const char *RST_TAG = "ResetReason";
#ifdef ENABLE_DIAGNOSTICS 
static const char *PT_TAG = "PerfTimer";
#endif

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

static void init_bno055(struct bno055_t *bno055){
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
        ESP_LOGI("BNO055_HAL", "BNO055 init OK! SW Rev ID: 0x%X, Chip ID: 0x%X", swRevId, chipId);
    } else {
        ESP_LOGE("BNO055_HAL", "BNO055 init error, current status: %d", result);
    }
}

// Task which runs on the master. Receives sensor data from slave and handles complex routines like FSM & BT.
static void master_task(void *pvParameter){
    static const char *TAG = "MasterTask";
    uint8_t robotId = 69;
    struct bno055_t bno055 = {0};
    float yaw = 0.0f;
    #ifdef ENABLE_DIAGNOSTICS
    int64_t worstTime = 0;
    int64_t bestTime = 0xFFFFF;
    movavg_t *avgTime = movavg_create(64);
    uint16_t ticks = 0;
    uint16_t ticksSinceLastWorstTime = 0;
    uint16_t ticksSinceLastBestTime = 0;
    #endif
    button_event_t buttonEvent = {0};
    float yawOffset = 0.0f;
    float yawRaw = 0.0f; // yaw before offset

    print_reset_reason();
    robotStateSem = xSemaphoreCreateMutex();

    // Initialise comms and hardware
    comms_uart_init();
    comms_i2c_init(I2C_NUM_1);
    i2c_scanner(I2C_NUM_1);
    cam_init();
    init_bno055(&bno055);
    bno055_convert_float_euler_h_deg(&yawRaw);
    yawOffset = yawRaw;
    ESP_LOGI(TAG, "Yaw offset: %f degrees", yawOffset);

    gpio_set_direction(KICKER_PIN, GPIO_MODE_OUTPUT);
    QueueHandle_t buttonQueue = button_init(PIN_BIT(RST_BTN));
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

    ESP_LOGI(TAG, "=============== Master software init OK ===============");
    ESP_LOGD(TAG, "Waiting on valid cam packet...");
    xSemaphoreTake(validCamPacket, portMAX_DELAY);
    ESP_LOGI(TAG, "Running!");
    esp_task_wdt_add(NULL);

    while (true){
        #ifdef ENABLE_DIAGNOSTICS
        int64_t begin = esp_timer_get_time();
        #endif

        // update sensors
        cam_calc();
        bno055_convert_float_euler_h_deg(&yawRaw);
        yaw = fmodf(yawRaw - yawOffset + 360.0f, 360.0f);

        // update values for FSM, mutexes are used to prevent race conditions
        if (xSemaphoreTake(robotStateSem, pdMS_TO_TICKS(SEMAPHORE_UNLOCK_TIMEOUT)) && 
            xSemaphoreTake(goalDataSem, pdMS_TO_TICKS(SEMAPHORE_UNLOCK_TIMEOUT))){
                // reset out values
                robotState.outShouldBrake = false;
                robotState.outOrientation = 0;
                robotState.outDirection = 0;
                robotState.outSwitchOk = false;

                // update FSM in values
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
                robotState.inX = robotX;
                robotState.inY = robotY;

                // unlock semaphores
                xSemaphoreGive(robotStateSem);
                xSemaphoreGive(goalDataSem);
        } else {
            ESP_LOGW(TAG, "Failed to acquire semaphores, cannot update FSM data.");
        }

        // update the actual FSM
        fsm_update(stateMachine);

        // Run acceleration
        // hmm_vec2 accel = calc_acceleration(robotState.outSpeed, robotState.outDirection);
        // ESP_LOGD(TAG, "Accel: (%f, %f)", accel.X, accel.Y);
        
        // encode and send Protobuf message to Teenys slave
        I2CMasterProvide msg = I2CMasterProvide_init_default;
        uint8_t buf[PROTOBUF_SIZE] = {0};
        pb_ostream_t stream = pb_ostream_from_buffer(buf, PROTOBUF_SIZE);

        // ESP_LOGD(TAG,"%d",robotState.inGoalAngle);
        // robotState.outSpeed = 0;
        // goal_correction(&robotState);
        // robotState.outDirection = 0;
        // print_ball_data(&robotState);
        
        msg.heading = yaw; // IMU heading
        msg.direction = robotState.outDirection; // motor direction (which way we're driving)
        msg.orientation = -robotState.outOrientation; // motor orientation (which way we're facing)
        msg.speed = robotState.outSpeed; // motor speed as 0-100%

        if (!pb_encode(&stream, I2CMasterProvide_fields, &msg)){
            ESP_LOGE(TAG, "I2C encode error: %s", PB_GET_ERROR(&stream));
        }
        comms_uart_send(MSG_PUSH_I2C_MASTER, buf, stream.bytes_written);

        if (xQueueReceive(buttonQueue, &buttonEvent, 0)){
            if ((buttonEvent.pin == RST_BTN) && (buttonEvent.event == BUTTON_UP)){
                ESP_LOGI(TAG, "Reset button pressed, resetting FSM & IMU...");
                fsm_dump(stateMachine);
                fsm_reset(stateMachine);

                // calculate new yaw offset
                bno055_convert_float_euler_h_deg(&yawRaw);
                yawOffset = yawRaw;
                ESP_LOGI(TAG, "New yaw offset: %f degrees", yawOffset);
            }
        }
        
        // main loop performance profiling code
        #ifdef ENABLE_DIAGNOSTICS
            int64_t end = esp_timer_get_time() - begin;
            movavg_push(avgTime, (float) end);
            ticks++;
            ticksSinceLastBestTime++;
            ticksSinceLastWorstTime++;

            if (end > worstTime){
                // we took longer than recorded previously, means we have a new worst time
                ESP_LOGW(PT_TAG, "New worst time: %ld us (last was %d ticks ago). Average time: %f us", 
                    (long) end, ticksSinceLastWorstTime, movavg_calc(avgTime));
                ticksSinceLastWorstTime = 0;
                worstTime = end;
            } else if (end < bestTime){
                // we took less than recorded previously, meaning we have a new best time
                ESP_LOGW(PT_TAG, "New best time: %ld us (last was %d ticks ago). Average time: %f us", 
                    (long) end, ticksSinceLastBestTime, movavg_calc(avgTime));
                ticksSinceLastBestTime = 0;
                bestTime = end;
            } else if (ticks >= 256){
                // print the average time and memory diagnostics every few loops
                ESP_LOGW(PT_TAG, "Average time: %f us", movavg_calc(avgTime));
                ESP_LOGW(PT_TAG, "Stack high usage: %d KB. Heap bytes free: %d KB (min free ever: %d KB)", 
                    uxTaskGetStackHighWaterMark(NULL) / 1000, esp_get_free_heap_size() / 1000,
                    esp_get_minimum_free_heap_size() / 1000);
                // fsm_dump(stateMachine);
                // ESP_LOGI(TAG, "Heading: %f", yaw);
                ticks = 0;
            }
        #endif

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
        // NVS partition was truncated and needs to be erased
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    nvs_handle storageHandle;
    ESP_ERROR_CHECK(nvs_open("RobotSettings", NVS_READWRITE, &storageHandle));

    // write master/slave/robot ID to NVS if configured
    #if defined NVS_WRITE_ROBOTNUM
        ESP_ERROR_CHECK(nvs_set_u8(storageHandle, "RobotID", NVS_WRITE_ROBOTNUM));
        ESP_ERROR_CHECK(nvs_commit(storageHandle));
        ESP_LOGE("RobotID", "Successfully wrote robot number to NVS.");
    #endif

    nvs_close(storageHandle);
    fflush(stdout);

    // create the main (or test, uncomment it if you want that) task 
    xTaskCreatePinnedToCore(master_task, "MasterTask", 16384, NULL, configMAX_PRIORITIES, NULL, APP_CPU_NUM);
    // xTaskCreatePinnedToCore(test_task, "TestTask", 8192, NULL, configMAX_PRIORITIES, NULL, APP_CPU_NUM);
}