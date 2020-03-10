#include "cam.h"

SemaphoreHandle_t goalDataSem = NULL;
cam_object_t goalBlue = {0};
cam_object_t goalYellow = {0};
cam_object_t orangeBall = {0};
float robotX = 0;
float robotY = 0;

// FIXME with Omicam this will now need to work with Protobuf - or recycle comms_uart.c
static void cam_receive_task(void *pvParameter){
    static const char *TAG = "CamReceiveTask";;
    
    uint8_t *buffer = calloc(CAM_BUF_SIZE, sizeof(uint8_t));
    ESP_LOGI(TAG, "Cam receive task init OK!");
    esp_task_wdt_add(NULL);

    while (true){
        memset(buffer, 0, CAM_BUF_SIZE);
        esp_task_wdt_reset();

        // wait slightly shorter than the watchdog timer for our bytes to come in
        uart_read_bytes(UART_NUM_2, buffer, CAM_BUF_SIZE, pdMS_TO_TICKS(4096)); 

        if (buffer[0] == CAM_BEGIN_BYTE){
            // xSemaphoreGive(validCamPacket);
            if (xSemaphoreTake(goalDataSem, pdMS_TO_TICKS(SEMAPHORE_UNLOCK_TIMEOUT))){
                // first byte is begin byte so skip that
                goalBlue.exists = buffer[1];
                goalBlue.x = buffer[2] - CAM_OFFSET_X;
                goalBlue.y = buffer[3] - CAM_OFFSET_Y;

                goalYellow.exists = buffer[4];
                goalYellow.x = buffer[5] - CAM_OFFSET_X;
                goalYellow.y = buffer[6] - CAM_OFFSET_Y;

                orangeBall.exists = buffer[7];
                orangeBall.x = buffer[8] - CAM_OFFSET_X;
                orangeBall.y = buffer[9] - CAM_OFFSET_Y;

                cam_calc();
                xSemaphoreGive(goalDataSem);
            } else {
                ESP_LOGW(TAG, "Unable to acquire goalDataSem!");
            }
        } else {
            ESP_LOGW(TAG, "Invalid buffer, first byte is: 0x%X, expected: 0x%X", buffer[0], CAM_BEGIN_BYTE);
        }

        uart_flush_input(UART_NUM_2);
        esp_task_wdt_reset();
    }
}

static const char *TAG = "Camera";

void cam_init(void){
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    // Configure UART parameters
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_2, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_2, CAM_UART_TX, CAM_UART_RX, -1, -1));
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_2, 256, 256, 8, NULL, 0));

    goalDataSem = xSemaphoreCreateMutex();
    // validCamPacket = xSemaphoreCreateMutex();
    // the main task will have to wait until a valid cam packet is received
    // xSemaphoreTake(validCamPacket, portMAX_DELAY);

    xTaskCreate(cam_receive_task, "CamReceiveTask", 4096, NULL, configMAX_PRIORITIES - 2, NULL);
    ESP_LOGI(TAG, "Camera init OK!");
}

/** Model: f(x) = 0.1937 * e^(0.0709x) */
static inline float cam_goal_pixel2cm(float measurement){
    return 0.1937f * powf(E, 0.0709f * measurement);
}

/** Model: f(x) = 0.0003 * x^2.8206 */
static inline float cam_ball_pixel2cm(float measurement){
    return 0.0003f * powf(measurement, 2.8206f);
}

void cam_calc(void){
    goalBlue.angle = floatMod(450.0f - roundf(RAD_DEG * atan2f(goalBlue.y, goalBlue.x)), 360.0f);
    goalBlue.length = sqrtf(sq(goalBlue.x) + sq(goalBlue.y));

    goalYellow.angle = floatMod(450.0f - roundf(RAD_DEG * atan2f(goalYellow.y, goalYellow.x)), 360.0f);
    goalYellow.length = sqrtf(sq(goalYellow.x) + sq(goalYellow.y));

    orangeBall.angle = floatMod(450.0f - roundf(RAD_DEG * atan2f(orangeBall.y, orangeBall.x)), 360.0f);
    orangeBall.length = sqrtf(sq(orangeBall.x) + sq(orangeBall.y));

    goalYellow.distance = cam_goal_pixel2cm(goalYellow.length);
    goalBlue.distance = cam_goal_pixel2cm(goalBlue.length);
    orangeBall.distance = cam_ball_pixel2cm(orangeBall.length);

    // ESP_LOGD(TAG, "[yellow] Pixel distance: %f\tActual distance: %f", goalYellow.length, goalYellow.distance);
    // ESP_LOGD(TAG, "[ball] Pixel distance: %f\tActual distance: %f", orangeBall.length, orangeBall.distance);
    // ESP_LOGD(TAG, "[ball] Pixel distance: %f", orangeBall.length);
    // vTaskDelay(pdMS_TO_TICKS(1000));
    // return;

    // basic localisation
    if (!goalBlue.exists && !goalYellow.exists){
        robotX = CAM_NO_VALUE;
        robotY = CAM_NO_VALUE;
    } else {
        cam_object_t *targetGoal = NULL;

        // select closest goal to localise on
        if (goalBlue.exists && !goalYellow.exists){
            // puts("Blue");
            targetGoal = &goalBlue;
        } else if (!goalBlue.exists && goalYellow.exists){
            // puts("Yellow");
            targetGoal = &goalYellow;
        } else {
            // puts(goalBlue.length < goalYellow.length ? "BLue" : "Yellow");
            targetGoal = goalBlue.length < goalYellow.length ? &goalBlue : &goalYellow;
        }

        // based on Aparaj's maths on the whiteboard
        float targetGoalAngle = fmodf((RAD_DEG * atan2f(targetGoal->y, targetGoal->x)) + 360.0f, 360.0f);
        robotX = targetGoal->x - targetGoal->distance * cosf(targetGoalAngle);
        robotY = targetGoal->y - targetGoal->distance * sinf(targetGoalAngle);

        // ESP_LOGD(TAG, "Robot position: (%f, %f)", robotX, robotY);
    }
}