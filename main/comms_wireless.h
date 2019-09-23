#pragma once
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "nvs.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_bt_device.h"
#include "esp_spp_api.h"
#include "defines.h"
#include "utils.h"
#include "pb.h"
#include "pb_encode.h"
#include "pb_decode.h"
#include "bluetooth.pb.h"
#include "math.h"
#include "freertos/task.h"
#include "esp_now.h"
#include "esp_wifi.h"

/** received packets are pushed into this queue and read by the BT manager task in bluetooth_manager.c */
extern QueueHandle_t packetQueue;
extern TaskHandle_t receiveTaskHandle;
extern TaskHandle_t sendTaskHandle;

/** Initialises the selected wireless stack as a slave, or initiator **/
void comms_wi_init_slave();
/** Initialises the selected wireless stack as a master, or acceptor **/
void comms_wi_init_master();
/** Wireless receive task */
void comms_wi_receive_task(void *pvParameter);
/** Wireless send task */
void comms_wi_send_task(void *pvParameter);
/** stops the send and receive tasks */
void comms_wi_stop_tasks(void);
/** reinits wireless **/
void comms_wi_reinit(void);