#include "utils.h"
// #include "esp_err.h"

// Hecking PIDs
// Orientation Correction PIDs
pid_config_t goalPID = {GOAL_KP, GOAL_KI, GOAL_KD, GOAL_MAX_CORRECTION, 0.0f};
pid_config_t headingPID = {HEADING_KP, HEADING_KI, HEADING_KD, HEADING_MAX_CORRECTION, 0.0f};
pid_config_t idlePID = {IDLE_KP, IDLE_KI, IDLE_KD, IDLE_MAX_CORRECTION, 0.0f};
pid_config_t goaliePID = {GOALIE_KP, GOALIE_KI, GOALIE_KD, GOALIE_MAX, 0.0f};

// Movement PIDs
pid_config_t coordPID = {COORD_KP, COORD_KI, COORD_KP, COORD_MAX, 0.0f};
pid_config_t sidePID = {SIDE_KP, SIDE_KI, SIDE_KD, SIDE_MAX, 0.0f};
pid_config_t forwardPID = {FORWARD_KP, FORWARD_KI, FORWARD_KD, FORWARD_MAX, 0.0f};

int32_t mod(int32_t x, int32_t m){
    int32_t r = x % m;
    return r < 0 ? r + m : r;
}

float floatMod(float x, float m) {
    float r = fmodf(x, m);
    return r < 0 ? r + m : r;
}

int number_comparator_descending(const void *a, const void *b){
    // descending order so b - a
    return (*(int*)b - *(int*)a);
}

// TODO rename to getAngleBetween
float angleBetween(float angleCounterClockwise, float angleClockwise){
    return mod(angleClockwise - angleCounterClockwise, 360);
}

float smallestAngleBetween(float angle1, float angle2){
    float ang = angleBetween(angle1, angle2);
    return fminf(ang, 360 - ang);
}

float midAngleBetween(float angleCounterClockwise, float angleClockwise){
    return mod(angleCounterClockwise + angleBetween(angleCounterClockwise, angleClockwise) / 2.0, 360);
}

inline int32_t map(int32_t x, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max){
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Source: https://stackoverflow.com/a/11412077/5007892 
bool is_angle_between(float target, float angle1, float angle2){
	// make the angle from angle1 to angle2 to be <= 180 degrees
	float rAngle = fmodf(fmodf(angle2 - angle1, 360.0f) + 360.0f, 360.0f);
	if (rAngle >= 180.0f){
		float a1 = angle1;
		angle1 = angle2;
		angle2 = a1;
	}

	// check if it passes through zero
	if (angle1 <= angle2){
		return target >= angle1 && target <= angle2;
	} else {
		return target >= angle1 || target <= angle2;
	}
}

void imu_correction(robot_state_t *robotState){
    if (robotState->outMotion.mag <= IDLE_MIN_SPEED){ // Check if robot is moving
        robotState->outOrientation = (int16_t) -pid_update(&idlePID, floatMod(floatMod((float)robotState->inHeading, 360.0f) 
                                + 180.0f, 360.0f) - 180.0f, 0.0f, 0.0f); // Correct with idle PID
        // printf("IDLE PID\n");
    } else {
        robotState->outOrientation = (int16_t) -pid_update(&headingPID, floatMod(floatMod((float)robotState->inHeading, 360.0f) 
                                + 180.0f, 360.0f) - 180, 0.0f, 0.0f); // Correct with heading PID
        // printf("HEADING PID\n");
    }
    
    // printf("IMU Correcting: %d\n", robotState->outOrientation);
}

void goal_correction(robot_state_t *robotState){
    if (robotState->inGoalVisible && robotState->inGoal.mag < GOAL_TRACK_DIST){
        // if the goal is visible use goal correction
        if (robotState->outIsAttack){ // If attacking
            robotState->outOrientation = (int16_t) pid_update(&goalPID, floatMod(floatMod((float)robotState->inGoal.arg, 360.0f) 
                                    + 180.0f, 360.0f) - 180.0f, 0.0f, 0.0f); // Use normal goal PID
            // printf("Attack goal correction");
        } else {
            robotState->outOrientation = (int16_t) pid_update(&goaliePID, floatMod(floatMod((float)robotState->inGoal.arg, 360.0f)
                                    , 360.0f) - 180.0f, 0.0f, 0.0f); // Use goalie PID. Also I don't remember how the hell this works but apparently it did
            // printf("Defend goal correction");
        }
    } else {
        // otherwise just IMU correct
        imu_correction(robotState);
        // printf("Cannot see goal");
    }
}

void other_goal_correction(robot_state_t *robotState){
    if (robotState->inOtherGoalVisible && robotState->inOtherGoal.mag < GOAL_TRACK_DIST){
        // if the goal is visible use goal correction
        if (!robotState->outIsAttack){ // If attacking
            robotState->outOrientation = (int16_t) pid_update(&goalPID, floatMod(floatMod((float)robotState->inOtherGoal.arg, 360.0f) 
                                    + 180.0f, 360.0f) - 180.0f, 0.0f, 0.0f); // Use normal goal PID
            // printf("Attack goal correction");
        } else {
            robotState->outOrientation = (int16_t) pid_update(&goaliePID, floatMod(floatMod((float)robotState->inOtherGoal.arg, 360.0f)
                                    , 360.0f) - 180.0f, 0.0f, 0.0f); // Use goalie PID. Also I don't remember how the hell this works but apparently it did
            // printf("Defend goal correction");
        }
    } else {
        // otherwise just IMU correct
        imu_correction(robotState);
        // printf("Cannot see goal");
    }
}

inline float get_magnitude(int16_t x, int16_t y){
    return sqrtf((float) (x * x + y * y)); // Cheeky pythag theorem
}

inline float get_angle(int16_t x, int16_t y){
    return fmodf(90 - RAD_DEG * (atan2(y, x)), 360.0f);
}

void move_by_difference(robot_state_t *robotState, int16_t x, int16_t y){
    if (get_magnitude(x, y) < COORD_THRESHOLD){
        robotState->outMotion = vect_2d(0, 0, false);
        robotState->outShouldBrake = true;
    }else{
        robotState->outMotion = vect_2d(fabsf(pid_update(&coordPID, get_magnitude(x, y), 0.0f, 0.0f)), fmodf(get_angle(x, y) - robotState->inHeading, 360.0f), true);
    }
}

// void move_to_xy(robot_state_t *robotState, int16_t x, int16_t y){
//     if (robotState->inGoalVisible){
//         return move_by_difference(robotState, x - robotState->inRobotPos.x, y - robotState->inRobotPos.y);
//     }else{
//         robotState->outShouldBrake = true;
//         robotState->outSpeed = 0;
//     }
// }

inline float lerp(float fromValue, float toValue, float progress){
    return fromValue + (toValue - fromValue) * progress;
}

// void orbit(robot_state_t *robotState){
//     // orbit requires angles in -180 to +180 range
//     float tempAngle = robotState->inBallAngle > 180 ? robotState->inBallAngle - 360 : robotState->inBallAngle;

//     // I hate to do this but...
//     if(robotState->inRobotId == 0){
//         // float tempStrength = is_angle_between(robotState->inBallAngle, 114.0f, 253.0f) ? robotState->inBallDistance * 1.45f : robotState->inBallDistance; // Stupid multiplier thing to incrase the distance on the sides cos it's too low
        
//         float ballAngleDifference = ((sign(tempAngle)) * fminf(90, 0.2 * powf(E, 0.1 * (float)smallestAngleBetween(tempAngle, 0)))); // Exponential function for how much extra is added to the ball angle
//         float distanceFactor = constrain(((float)robotState->inBallDistance - (float)BALL_FAR_STRENGTH) / ((float)BALL_CLOSE_STRENGTH - BALL_FAR_STRENGTH), 0, 1); // Scale distance between 0 and 1
//         float distanceMultiplier = constrain(0.08 * distanceFactor * powf(E, 3.8 * distanceFactor), 0, 1); // Use that to make another exponential function based on distance
//         float angleAddition = ballAngleDifference * distanceMultiplier; // Multiply them together (distance multiplier will affect the angle difference)

//         robotState->outDirection = floatMod(robotState->inBallAngle + angleAddition, 360);
//         // robotState->outSpeed = ORBIT_SPEED_SLOW + (float)(ORBIT_SPEED_FAST - ORBIT_SPEED_SLOW) * (1.0 - (float)fabsf(angleAddition) / 90.0);
//         robotState->outSpeed = lerp((float)ORBIT_SPEED_SLOW, (float)ORBIT_SPEED_FAST, (1.0 - (float)fabsf(angleAddition) / 90.0)); // Linear interpolation for speed
//         // printf("tempStrength: %f\n", tempStrength);
//     } else {
//         // float tempStrength = is_angle_between(robotState->inBallAngle, 114.0f, 253.0f) ? robotState->inBallDistance * 1.45f : robotState->inBallDistance; // Stupid multiplier thing to incrase the distance on the sides cos it's too low
        
//         float ballAngleDifference = ((sign(tempAngle)) * fminf(90, 0.2 * powf(E, 0.1 * (float)smallestAngleBetween(tempAngle, 0)))); // Exponential function for how much extra is added to the ball angle
//         float distanceFactor = constrain(((float)robotState->inBallDistance - (float)BALL_FAR_STRENGTH) / ((float)BALL_CLOSE_STRENGTH - BALL_FAR_STRENGTH), 0, 1); // Scale distance between 0 and 1
//         float distanceMultiplier = constrain(0.08 * distanceFactor * powf(E, 3.8 * distanceFactor), 0, 1); // Use that to make another exponential function based on distance
//         float angleAddition = ballAngleDifference * distanceMultiplier; // Multiply them together (distance multiplier will affect the angle difference)

//         robotState->outDirection = floatMod(robotState->inBallAngle + angleAddition, 360);
//         // robotState->outSpeed = ORBIT_SPEED_SLOW + (float)(ORBIT_SPEED_FAST - ORBIT_SPEED_SLOW) * (1.0 - (float)fabsf(angleAddition) / 90.0);
//         robotState->outSpeed = lerp((float)ORBIT_SPEED_SLOW, (float)ORBIT_SPEED_FAST, (1.0 - (float)fabsf(angleAddition) / 90.0)); // Linear interpolation for speed
//         // printf("tempStrength: %f\n", tempStrength);
//     }

    // ESP_LOGD(TAG, "Ball is visible, orbiting");
    // printf("ballAngleDifference: %f, distanceFactor: %f, distanceMultiplier: %f, angleAddition: %f\n", ballAngleDifference, 
    // distanceFactor, distanceMultiplier, angleAddition);
// }

void position(robot_state_t *robotState, float distance, float offset, int16_t goalAngle, int16_t goalLength, bool reversed) {
    float goalAngle_ = goalAngle < 0.0f ? goalAngle + 360.0f : goalAngle; // Convert to 0 - 360 range
    float goalAngle__ = fmodf(goalAngle_ + robotState->inHeading, 360.0f); // Add the heading to counteract the rotation

    float verticalDistance = fabsf(goalLength * cosf(DEG_RAD * goalAngle__)); // Break the goal vector into cartesian components (not actually vectors but it kinda is)
    float horizontalDistance = goalLength * sinf(DEG_RAD * goalAngle__);

    float distanceMovement = -pid_update(&forwardPID, verticalDistance, distance, 0.0f); // Determine the speed for each component
    float sidewaysMovement = -pid_update(&sidePID, horizontalDistance, offset, 0.0f);

    if(reversed) distanceMovement *= -1; // All dimensions are inverted cos the goal is behind for the defender

    robotState->outMotion = vect_2d(get_magnitude(sidewaysMovement, distanceMovement) <= IDLE_MIN_SPEED ? 0 : robotState->outMotion.mag, fmodf(RAD_DEG * (atan2f(sidewaysMovement, distanceMovement)) - robotState->inHeading, 360.0f), true); // Use atan2 to find angle
   // Use pythag to find the overall speed

     // To stop the robot from spazzing, if the robot is close to it's destination (so is moving very little), it will just stop.

    // printf("goalAngle_: %f, verticalDistance: %f, horizontalDistance: %f\n", goalAngle_, verticalDistance, horizontalDistance);
    // printf("goalAngle_: %f, verticalDistance: %f, distanceMovement: %f, horizontalDistance: %f, sidewaysMovement: %f\n", goalAngle_, verticalDistance, distanceMovement, horizontalDistance, sidewaysMovement);
}

void nvs_get_u8_graceful(char *namespace, char *key, uint8_t *value){
    static const char *TAG = "NVSGetU8";
    nvs_handle storageHandle;

    ESP_ERROR_CHECK(nvs_open(namespace, NVS_READWRITE, &storageHandle));
    esp_err_t openErr = nvs_get_u8(storageHandle, key, value);
    nvs_close(storageHandle);

    if (openErr == ESP_ERR_NVS_NOT_FOUND){
        ESP_LOGE(TAG, "Key \"%s\" not found in namespace %s! Please set it in NVS, see top of defines.h for help. "
                "Cannot continue.", key, namespace);
        abort(); // not very graceful lmao
    } else if (openErr != ESP_OK) {
        ESP_LOGE(TAG, "Unexpected error reading key \"%s\": %s. Cannot continue.", key, esp_err_to_name(openErr));
        abort();
    }
}

void update_line(robot_state_t *robotState) { // TODO: FIX THIS
    // robotState->inOnLine = robotState->inLineAngle != NO_LINE_ANGLE;

    // if (robotState->inOnField) {
    //     if (robotState->inOnLine) {
    //         robotState->inOnField = false;
    //         robotState->inTrueLineAngle = robotState->inLineAngle;
    //         robotState->inTrueLineSize = robotState->inLineSize;
    //     }
    // } else {
    //     if (robotState->inTrueLineAngle == 3) {
    //         if (robotState->inOnLine) {
    //             robotState->inTrueLineAngle = modf(robotState->inLineAngle + 180.0f, 360.0f);
    //             robotState->inTrueLineSize = 2 - robotState->inLineSize;
    //         }
    //     } else {
    //         if (!robotState->inOnLine) {
    //             if (robotState->inTrueLineSize <= 1) {
    //                 robotState->inOnField = true;
    //                 robotState->inTrueLineSize = -1;
    //                 robotState->inTrueLineAngle = NO_LINE_ANGLE;
    //             } else {
    //                 robotState->inTrueLineSize = 3;
    //             }
    //         } else {
    //             if (smallestAngleBetween(robotState->inLineAngle, robotState->inTrueLineAngle) <= LS_LINEOVER_BUFFER) {
    //                 robotState->inTrueLineAngle = robotState->inLineAngle;
    //                 robotState->inTrueLineSize = robotState->inLineSize;
    //             } else {
    //                 robotState->inTrueLineAngle = modf(robotState->inLineAngle + 180.0f, 360.0f);
    //                 robotState->inLineSize = 2 - robotState->inLineSize;
    //             }
    //         }
    //     }
    // }
}

hmm_vec2 vec2_polar_to_cartesian(hmm_vec2 vec){
    // r cos theta, r sin theta
    // where r = X, theta = Y
    float r = vec.X;
    float theta = vec.Y;
    return HMM_Vec2(r * cosfd(theta), r * sinfd(theta));
}

hmm_vec2 vec2_cartesian_to_polar(hmm_vec2 vec){
    // from TSOP code: sqrtf(sq(sumX) + sq(sumY)), fmodf((atan2f(sumY, sumX) * RAD_DEG) + 360.0f, 360.0f)
    float x = vec.X;
    float y = vec.Y;
    return HMM_Vec2(sqrtf(sq(x) + sq(y)), fmodf((atan2f(y, x) * RAD_DEG) + 360.0f, 360.0f));
}

void i2c_scanner(i2c_port_t port){
    ESP_LOGI("I2CScanner", "Scanning port %d...", port);

    int i;
	esp_err_t espRc;
	printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");
	printf("00:         ");
	for (i=3; i< 0x78; i++) {
		i2c_cmd_handle_t cmd = i2c_cmd_link_create();
		i2c_master_start(cmd);
		i2c_master_write_byte(cmd, (i << 1) | I2C_MASTER_WRITE, 1 /* expect ack */);
		i2c_master_stop(cmd);

		espRc = i2c_master_cmd_begin(port, cmd, 75 / portTICK_PERIOD_MS);
		if (i%16 == 0) {
			printf("\n%.2x:", i);
		}
		if (espRc == 0) {
			printf(" %.2x", i);
		} else {
			printf(" --");
		}
		// ESP_LOGD("I2CScanner", "Addr 0x%X, RC %s", i, esp_err_to_name(espRc));
		i2c_cmd_link_delete(cmd);
	}
	printf("\n");
}

void print_ball_data(robot_state_t *robotState){
    static const char *TAG = "BallDebug";
    ESP_LOGD(TAG, "Ball angle: %f, Ball distance: %f", robotState->inBallPos.arg, robotState->inBallPos.mag);
}

void print_line_data(robot_state_t *robotState){
    static const char *TAG = "LineDebug";
    // ESP_LOGD(TAG, "Line angle: %f, Line size: %f, On line: %d, On field: %d, Last angle: %f", robotState->inLineAngle, 
    // robotState->inLineSize, robotState->inOnLine, robotState->inOnField, robotState->inLastAngle);
    ESP_LOGD(TAG, "Currently obselete, needs fixing once I figure out how the rest of this LS stuff works");
}

void print_goal_data(){
    static const char *TAG = "GoalDebug";
    ESP_LOGW(TAG, "print_goal_data() is currently not working, ping Matt if you need a fix");
    // ESP_LOGD(TAG, "Yellow - Angle: %f, Length: %f, Distance: %f | Blue - Angle: %f, Length: %f, Distance: %f", goalYellow.angle, 
    // goalYellow.length, goalYellow.distance, goalBlue.angle, goalBlue.length, goalBlue.distance);
}

void print_position_data(robot_state_t *robotState){
    static const char *TAG = "PositionDebug";
    ESP_LOGD(TAG, "Xpos: %f, Ypos: %f, Heading: %f", robotState->inRobotPos.x, robotState->inRobotPos.y, robotState->inHeading);
}

void print_motion_data(robot_state_t *robotState){
    static const char *TAG = "MotionDebug";
    ESP_LOGD(TAG, "Speed: %f, Direction: %f, Orientation: %f, Should break: %d", robotState->outMotion.mag, robotState->outMotion.arg, 
    robotState->outOrientation, robotState->outShouldBrake);
}

// code adapted from: https://en.wikipedia.org/wiki/Jenkins_hash_function
uint32_t str_hash(char *str){
    size_t i = 0;
    uint32_t hash = 0;
    size_t length = strlen(str);

    while (i != length) {
        hash += str[i++];
        hash += hash << 10;
        hash ^= hash >> 6;
    }
    hash += hash << 3;
    hash ^= hash >> 11;
    hash += hash << 15;
    return hash;
}

#define LOGGED_MSG_SIZE 32
static uint8_t msgIndex = 0;
// hash of messages which have already been logged, we use hash instead of strcmp for speed
// yes, hash collisions are possible but this is unlikely/we don't care too much
static uint32_t loggedMessages[LOGGED_MSG_SIZE] = {0};

bool log_once_check(char *msg){
    uint32_t hash = str_hash(msg);

    // we compare messages before formatting, dropping those which have already been printed
    for (int i = 0; i < LOGGED_MSG_SIZE; i++){
        if (loggedMessages[i] == hash){
            // printf("Found \"%s\" at %d (found hash: %d, my hash: %d)\n", msg, i, loggedMessages[i], hash);
            return false;
        }
    }

    // the message hasn't been printed already, so add it to the list
    // printf("\"%s\" is not in array, hash %d added at %d\n", msg, hash, msgIndex);
    loggedMessages[msgIndex++] = hash;
    return true;
}

void log_once_reset(){
    msgIndex = 0;
    memset(loggedMessages, 0, LOGGED_MSG_SIZE);
}

s8 bno055_read(u8 dev_addr, u8 reg_addr, u8 *reg_data, u8 cnt){
    static const char *TAG = "BNO055_HAL";

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    ESP_ERROR_CHECK(i2c_master_start(cmd));
    // first, send device address (indicating write) & register to be read
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (dev_addr << 1), I2C_ACK_MODE));
    // send register we want
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, reg_addr, I2C_ACK_MODE));
    // Send repeated start
    ESP_ERROR_CHECK(i2c_master_start(cmd));
    // now send device address (indicating read) & read data
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_READ, I2C_ACK_MODE));
    if (cnt > 1) {
        ESP_ERROR_CHECK(i2c_master_read(cmd, reg_data, cnt - 1, 0x0));
    }
    ESP_ERROR_CHECK(i2c_master_read_byte(cmd, reg_data + cnt - 1, 0x1));
    ESP_ERROR_CHECK(i2c_master_stop(cmd));
    esp_err_t ret = i2c_master_cmd_begin(BNO_BUS, cmd, pdMS_TO_TICKS(I2C_TIMEOUT));
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK){
        ESP_LOGE(TAG, "I2C failure in bno55_read: %s. reg_addr: 0x%X, cnt: %d. reg_data follows:", 
                esp_err_to_name(ret), reg_addr, cnt);
        ESP_LOG_BUFFER_HEX_LEVEL(TAG, reg_data, cnt, ESP_LOG_ERROR);
        i2c_reset_tx_fifo(I2C_NUM_1);
        i2c_reset_rx_fifo(I2C_NUM_1);
        return ret;
    }
    return 0;
}

s8 bno055_write(u8 dev_addr, u8 reg_addr, u8 *reg_data, u8 cnt){
    static const char *TAG = "BNO055_HAL";

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    ESP_ERROR_CHECK(i2c_master_start(cmd));

    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_WRITE, I2C_ACK_MODE));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, reg_addr, true));
    ESP_ERROR_CHECK(i2c_master_write(cmd, reg_data, cnt, true));
    ESP_ERROR_CHECK(i2c_master_stop(cmd));

    esp_err_t ret = i2c_master_cmd_begin(BNO_BUS, cmd, pdMS_TO_TICKS(I2C_TIMEOUT));
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK){
        ESP_LOGE(TAG, "I2C failure in bno055_write: %s. reg_addr: 0x%X, cnt: %d. reg_data follows:", 
                esp_err_to_name(ret), reg_addr, cnt);
        ESP_LOG_BUFFER_HEX_LEVEL(TAG, reg_data, cnt, ESP_LOG_ERROR);
        i2c_reset_tx_fifo(I2C_NUM_1);
        i2c_reset_rx_fifo(I2C_NUM_1);
        return ret;
    }
    return 0;
}

void bno055_delay_ms(u32 msec){
    // ets_delay_us(msec / 1000); // (if precision is required)
    vTaskDelay(pdMS_TO_TICKS(msec));
}

uint8_t nano_read(uint8_t addr, size_t size, uint8_t *data, robot_state_t *robotState) {
    static const char *TAG = "NanoRead";
    uint8_t sendBytes[] = {0xB, robotState->outFRMotor + 100, robotState->outBRMotor + 100, -robotState->outBLMotor + 100, robotState->outFLMotor + 100};
    
    
    // ESP_LOGI(TAG, "WRITING TO NANO");
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1), I2C_ACK_MODE);
    i2c_master_write(cmd, sendBytes, 5, I2C_ACK_MODE);
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

    // I2C_ERR_CHECK(ret);
    return ESP_OK;
}

static hmm_vec2 current = {0};
hmm_vec2 calc_acceleration(float speed, float direction){
    // 1. Calculate target vector
    // Direction is currently an angle from north but needs to be an angle from the X axis
    float newDir = fmodf(direction - 450.0f, 360.0f) * -1;
    // Create polar vector with direction and speed
    hmm_vec2 target = HMM_Vec2(speed / 100.0f, newDir);
    target = vec2_polar_to_cartesian(target);

    // 2. Calculate output vector
    // (current * (1 - MAX_ACCELERATION)) + (target * MAX_ACCELERATION);
    hmm_vec2 output = HMM_AddVec2(HMM_MultiplyVec2f(current, 1.0f - MAX_ACCELERATION), 
                                    HMM_MultiplyVec2f(target, MAX_ACCELERATION));

    current = output;
    
    // 3. Convert back to polar
    hmm_vec2 outPolar = vec2_cartesian_to_polar(output);
    outPolar.X = constrain(outPolar.X * 100.0f, 0.0f, 100.0f); // scale back to 0-100 speed
    outPolar.Y = fmodf(outPolar.Y * -1 + 450.0f, 360.0f);
    return outPolar;
}