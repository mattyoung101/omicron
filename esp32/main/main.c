// ATTENTION: PRE-PROCESSOR VOODOO MAGIC BULLSHIT TO MAKE SINGLE HEADER LIBRARIES WORK
// DO NOT fucking touch this shit or the entire build process will break, it must stay EXACTLY AS IS!!!
#define HANDMADE_MATH_IMPLEMENTATION
#define HANDMADE_MATH_NO_SSE
#define DG_DYNARR_IMPLEMENTATION
#include "HandmadeMath.h"
#include "DG_dynarr.h"
#undef HANDMADE_MATH_IMPLEMENTATION
#undef DG_DYNARR_IMPLEMENTATION
// END BULLSHIT (you can change from here onwards)
#define _GNU_SOURCE
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
#include "states.h"
#include "comms_i2c.h"
#include "comms_uart.h"
#include "esp_timer.h"
#include "esp_task_wdt.h"
#include "pid.h"
#include "comms_bluetooth.h"
#include "pb_encode.h"
#include "pb_decode.h"
#include "wirecomms.pb.h"
#include "bno055.h"
#include "button.h"
#include "movavg.h"
#include "goap.h"
#include "path_following.h"
#include "buzzer.h"
#include "Vector.h"
#include "motor.h"
#include "isosb.h"
#include "lsavoid.h"

typedef struct {
    bool exists;
    vect_2d_t vec;
} cam_object_t;

static cam_object_t goalYellow;
static cam_object_t goalBlue;

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

SemaphoreHandle_t robotStateSem;

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
    float yawOffset = 0.0f;
    float yawRaw = 0.0f; // yaw before offset

    print_reset_reason();
    robotStateSem = xSemaphoreCreateBinary();
    xSemaphoreGive(robotStateSem);

    // Initialise comms and hardware
    comms_uart_init(SBC_CAMERA);
    comms_uart_init(MCU_TEENSY);

    comms_i2c_init_bno(I2C_NUM_1);
    comms_i2c_init_nano(I2C_NUM_0);
    // i2c_scanner(I2C_NUM_0);
    // i2c_scanner(I2C_NUM_1);
    init_bno055(&bno055);
    bno055_convert_float_euler_h_deg(&yawRaw);
    yawOffset = yawRaw;
    ESP_LOGI(TAG, "Yaw offset: %f degrees", yawOffset);

    // TODO kicker pins
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
    stateMachine = fsm_new(&stateDefenceDefend);
#else
#if DEFENCE
    stateMachine = fsm_new(&stateDefenceDefend);
#else
    stateMachine = fsm_new(&stateAttackPursue);
#endif
#endif

    ESP_LOGI(TAG, "=============== Master software init OK ===============");
    // FIXME uncomment this once UART is working
    // ESP_LOGI(TAG, "Waiting on valid cam packet...");
    // xSemaphoreTake(validCamPacket, portMAX_DELAY);
    // ESP_LOGI(TAG, "Cam packet received. Running!");

    esp_task_wdt_add(NULL);

    while (true){
#ifdef ENABLE_DIAGNOSTICS
        int64_t begin = esp_timer_get_time();
#endif
        // update sensors
        bno055_convert_float_euler_h_deg(&yawRaw);
        yaw = fmodf(yawRaw - yawOffset + 360.0f, 360.0f);
        // printf("yaw: %.2f\n", yaw);

        // update values for FSM, mutexes are used to prevent race conditions
        if (xSemaphoreTake(robotStateSem, pdMS_TO_TICKS(SEMAPHORE_UNLOCK_TIMEOUT)) && 
            xSemaphoreTake(uartDataSem, pdMS_TO_TICKS(SEMAPHORE_UNLOCK_TIMEOUT))){
                // copy across data from object data message
                goalYellow.exists = lastObjectData.goalYellowExists;
                goalYellow.vec = vect_2d(lastObjectData.goalYellowMag, lastObjectData.goalYellowAngle, true);
                goalBlue.exists = lastObjectData.goalBlueExists;
                goalBlue.vec = vect_2d(lastObjectData.goalBlueMag, lastObjectData.goalBlueAngle, true);

                // reset out values
                robotState.outShouldBrake = false;
                robotState.outOrientation = 0;
                robotState.outMotion = vect_2d(0, 0, false);
                robotState.outSwitchOk = false;

                // light sensor stuff
                robotState.inLineAngle = lastLSlaveData.lineAngle;
                robotState.inLineSize = lastLSlaveData.lineSize;

                // update FSM in values
                robotState.inBallPos = lastObjectData.ballExists ? 
                                vect_2d(lastObjectData.ballMag, lastObjectData.ballAngle, true) : 
                                vect_2d(0.0f, 0.0f, true);
                robotState.inBallVisible = lastObjectData.ballExists;

                if (robotState.outIsAttack){
                    robotState.inGoalVisible = AWAY_GOAL.exists;
                    robotState.inGoal = AWAY_GOAL.vec;

                    robotState.inOtherGoalVisible = HOME_GOAL.exists;
                    robotState.inOtherGoal = HOME_GOAL.vec;
                } else {
                    robotState.inOtherGoalVisible = AWAY_GOAL.exists;
                    robotState.inOtherGoal = AWAY_GOAL.vec;

                    robotState.inGoalVisible = HOME_GOAL.exists;
                    robotState.inGoal = HOME_GOAL.vec;
                }
                robotState.inHeading = yaw;
                robotState.inRobotPos = vect_2d(lastLocaliserData.estimatedX + nanoData.dx, lastLocaliserData.estimatedY + nanoData.dy, false);

                // Testing LED thing
                robotState.debugLEDs[0] = true;
                robotState.debugLEDs[1] = false;

                // unlock semaphores
                xSemaphoreGive(robotStateSem);
                xSemaphoreGive(uartDataSem);
        } else {
            ESP_LOGW(TAG, "Failed to acquire semaphores, cannot update FSM data.");
        }

        // Hey matt shouldn't all this crap go into a semaphore?

        // update the actual FSM
        fsm_update(stateMachine);

        RS_SEM_LOCK
        movement_avoid_line(vect_2d(robotState.inLineSize, robotState.inLineAngle, true));

        // calculates motor values
        if(robotState.inLineSize > 0.0f){
            motor_calc(robotState.outMotion.arg, robotState.outOrientation, robotState.outMotion.mag);
        } else {
            motor_calc(0, robotState.outOrientation, 0);
        }
        RS_SEM_UNLOCK
        
        // encode and send Protobuf message to Teensy slave
        MasterToLSlave teensyMsg = MasterToLSlave_init_default;
        teensyMsg.heading = yaw;
        memcpy(teensyMsg.debugLEDs, robotState.debugLEDs, 6 * sizeof(bool));

        uint8_t teensyBuf[PROTOBUF_SIZE] = {0};
        pb_ostream_t teensyStream = pb_ostream_from_buffer(teensyBuf, PROTOBUF_SIZE);
        if (!pb_encode(&teensyStream, MasterToLSlave_fields, &teensyMsg)){
            ESP_LOGW(TAG, "Failed to encode Teensy protobuf message: %s", PB_GET_ERROR(&teensyStream));
        }
        comms_uart_send(MCU_TEENSY, MSG_ANY, teensyBuf, teensyStream.bytes_written);

        // encode and send Protobuf message to Omicam
        SensorData sensorData = SensorData_init_default;
        sensorData.orientation = yaw;
        sensorData.absDspX = robotState.inRobotPos.x;
        sensorData.absDspY = robotState.inRobotPos.y;
        sensorData.relDspX = nanoData.dx;
        sensorData.relDspY = nanoData.dy;
        if (xSemaphoreTake(stateMachine->updateInProgress, pdMS_TO_TICKS(SEMAPHORE_UNLOCK_TIMEOUT))){
            char *stateName = fsm_get_current_state_name(stateMachine);
            strcpy(sensorData.fsmState, stateName);
            free(stateName);
            xSemaphoreGive(stateMachine->updateInProgress);
        } else {
            ESP_LOGE(TAG, "Failed to unlock updateInProgress semaphore, cannot get current state name");
        }
        
        uint8_t camBuf[PROTOBUF_SIZE] = {0};
        pb_ostream_t camStream = pb_ostream_from_buffer(camBuf, PROTOBUF_SIZE);
        if (!pb_encode(&camStream, SensorData_fields, &sensorData)){
            ESP_LOGE(TAG, "Failed to encode Omicam protobuf message: %s", PB_GET_ERROR(&camStream));
        }
        comms_uart_send(SBC_CAMERA, SENSOR_DATA, camBuf, camStream.bytes_written);
        
        // main loop performance profiling code
#ifdef ENABLE_DIAGNOSTICS
        int64_t end = esp_timer_get_time() - begin;
        movavg_push(avgTime, (float) end);
        ticks++;
        ticksSinceLastBestTime++;
        ticksSinceLastWorstTime++;
        float avgFreq = (1.0f / movavg_calc(avgTime)) * 1000000.0f;

        if (end > worstTime){
            // we took longer than recorded previously, means we have a new worst time
            ESP_LOGW(PT_TAG, "New worst time: %ld us (last was %d ticks ago). Average time: %.2f us (%.2f Hz)", 
                (long) end, ticksSinceLastWorstTime, movavg_calc(avgTime), avgFreq);
            ticksSinceLastWorstTime = 0;
            worstTime = end;
        } else if (end < bestTime){
            // we took less than recorded previously, meaning we have a new best time
            ESP_LOGW(PT_TAG, "New best time: %ld us (last was %d ticks ago). Average time: %.2f us (%.2f Hz)", 
                (long) end, ticksSinceLastBestTime, movavg_calc(avgTime), avgFreq);
            ticksSinceLastBestTime = 0;
            bestTime = end;
        } else if (ticks >= 512){
            // print the average time and memory diagnostics every few loops
            ESP_LOGW(PT_TAG, "Average time: %.2f us (%.2f Hz). Heap bytes free: %d KB (min free ever: %d KB)", 
                    movavg_calc(avgTime), avgFreq, esp_get_free_heap_size() / 1024, 
                    esp_get_minimum_free_heap_size() / 1024);
            // fsm_dump(stateMachine);
            // ESP_LOGI(TAG, "Heading: %f", yaw);
            ticks = 0;
        }
#endif
        esp_task_wdt_reset();
        // is this necessary anymore?
        // No - Ethan
        // thanks, got rid of it - Matt
        // vTaskDelay(pdMS_TO_TICKS(50)); // Random delay at of loop to allow motors to spin OR TO LET ME READ THE BLOODY LOG
    }
}

// static void test_music_task(void *pvParameter){
//     static const char *TAG = "TestMusicTask";
//     buzzer_init();
//     while(true){
//         play_song(10);
//     }
// }

static void responding_timer(TimerHandle_t handle){
    puts("Device is responding.");
}

void app_main(){
    puts("====================================================================================");
    puts(" * This ESP32 belongs to a robot from Team Omicron at Brisbane Boys' College.");
    puts(" * Software copyright (c) 2019-2020 Team Omicron. All rights reserved.");
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
        ESP_LOGW("RobotID", "Successfully wrote robot number to NVS.");
    #endif

    nvs_close(storageHandle);
    fflush(stdout);
    fflush(stderr);

    TimerHandle_t respondTimer = xTimerCreate("RespondTimer", pdMS_TO_TICKS(5000), true, NULL, responding_timer);
    xTimerStart(respondTimer, pdMS_TO_TICKS(250));

    // create the main (or test, uncomment it if you want that) task 
    xTaskCreatePinnedToCore(master_task, "MasterTask", 16384, NULL, configMAX_PRIORITIES, NULL, APP_CPU_NUM);
    // xTaskCreatePinnedToCore(isosb_test_task, "ISOSB", 16384, NULL, configMAX_PRIORITIES, NULL, APP_CPU_NUM);
    // xTaskCreatePinnedToCore(test_music_task, "TestMusicTask", 8192, NULL, configMAX_PRIORITIES, NULL, APP_CPU_NUM);
}