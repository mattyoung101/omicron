#pragma once
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// If this is defined, the value of the robot number will be written to NVS
// #define NVS_WRITE_ROBOTNUM 1 // 0 or 1, 0 = bluetooth acceptor (master), 1 = bluetooth initiator (slave)

// FreeRTOS
#define SEMAPHORE_UNLOCK_TIMEOUT 25 // ms
#define CONF_LOG_LEVEL ESP_LOG_DEBUG

// Bluetooth
#define ROBOT0_NAME "Omicron_Robot0"
#define ROBOT1_NAME "Omicron_Robot1"
#define SPP_NAME "Omicron_SPP"
#define PACKET_QUEUE_LENGTH 1 // length of BT packet queue, currently 1 as we use xQueueOverwrite
#define BT_CONF_RES_STATIC 0 // uses pre-defined roles to resolve conflicts
#define BT_CONF_RES_DYNAMIC 1 // uses ball data to resolve conflicts
#define BT_PACKET_TIMEOUT 1500 // ms, if we haven't received a packet in this long, other robot is off for damage
#define BT_SWITCH_COOLDOWN 2500 // ms, wait this many ms after a switch before another switch is allowed
#define BLUETOOTH_ENABLED // whether or not Bluetooth is enabled
// #define BT_SWITCHING_ENABLED // if Bluetooth role switching is enabled or not (off for damage detection aways runs)
#define BT_CONF_RES_MODE BT_CONF_RES_DYNAMIC // the conflict resolution mode
#define DEFENCE false // whether to start out in defence (I think? unclear)
#define BT_MAX_ERRORS 4 // max errors before dropping connection

// Debug
#define ENABLE_DEBUG // if this is defined, debug features are enabled
#ifdef ENABLE_DEBUG
    // #define ENABLE_DIAGNOSTICS // if this is defined then performance and RAM diagnostics will be printed
    // #define ENABLE_VERBOSE_BT // verbose BT logging
#endif

// Acceleration
#define MAX_ACCELERATION 0.001f // must be between 0.0 and 1.0

// I2C
#define I2C_SLAVE_DEV_ADDR 0x23 // the I2C address of the Teensy slave
#define I2C_TIMEOUT 250 // ms
#define I2C_ACK_MODE 0x1 // 0x0 to disable ack
#define I2C_BEGIN_DEFAULT 0xB // default packet, has sensor data
#define I2C_BEGIN_DEBUG 0xC // debug packet, has raw data for sending to webserver
#define I2C_SLAVE_DEV_BUS I2C_NUM_0 // which bus the Teensy slave is on (I2C_NUM_0 or I2C_NUM_1)

// Goals
#define GOAL_YELLOW 0
#define GOAL_BLUE 1
#define GOAL_OFF 2
#define HALFWAY_DISTANCE 45
#define COORD_THRESHOLD 0
#define GOAL_TRACK_DIST 10000 // If the goal distance is less than this, track the goal
#define IDLE_MIN_SPEED 0 // The lowest speed for which the robot will move while positioning
#define GOAL_TOO_CLOSE 30
#define GOAL_WIDTH 40
#define ENEMY_GOAL GOAL_YELLOW

// Protobuf
#define PROTOBUF_SIZE 64 // size of protobuf input/output buffer, make it a safe size to avoid buffer overflows
#define I2C_BUF_SIZE 128 // size of I2C buffer
typedef enum {
    MSG_PUSH_I2C_SLAVE = 0, // as the I2C slave, I'm providing data to the I2C master
    MSG_PUSH_I2C_MASTER, // as the I2C master, I'm providing data to the I2C slave
    MSG_PULL_I2C_SLAVE, // requesting data from the I2C slave
} msg_type_t;

// Music
#define MUSIC_BPM 100

// PIDs
// --- IMU Correction --- //
// Note: this needs to be reversed (-pid_update)
#define HEADING_KP 0.8
#define HEADING_KI 0
#define HEADING_KD 0.1
#define HEADING_MAX_CORRECTION 100

#define LINEAVOID_KP 100
#define LINEAVOID_KI 0
#define LINEAVOID_KD 0
#define LINEAVOID_MAX 60

// --- Idle Correction --- //
#define IDLE_KP 0.8
#define IDLE_KI 0
#define IDLE_KD 0.05
#define IDLE_MAX_CORRECTION 100

// --- Goalie PIDs --- //
// PID which controls the robot moving to its correct distance from the goal
// TODO increase P of forward PID so its faster to get back into goal for example after surging
#define FORWARD_KP 4
#define FORWARD_KI 0
#define FORWARD_KD 0
#define FORWARD_MAX 100

// PID which controls the robot centering on the goal
#define SIDE_KP 1.2
#define SIDE_KI 0
#define SIDE_KD 0
#define SIDE_MAX 100

// PID which controls the robot going to intercept the ball
#define INTERCEPT_KP 1.5
#define INTERCEPT_KI 0
#define INTERCEPT_KD 0.00005
#define INTERCEPT_MAX 100
#define INTERCEPT_MIN 0

// PID which controls the robot turning its back towards the goal
#define GOALIE_KP 1.5
#define GOALIE_KI 0
#define GOALIE_KD 0.1
#define GOALIE_MAX 100

// --- Coordinate PID --- //
// Note: doesn't fucking work
#define COORD_KP 5
#define COORD_KI 0
#define COORD_KD 0.2
#define COORD_MAX 100

#define LRF_KP 1
#define LRF_KI 0
#define LRF_KD 0.1
#define LRF_MAX 100

// --- Goal Correction --- //
#define GOAL_KP 0.5
#define GOAL_KI 0
#define GOAL_KD 0.05
#define GOAL_MAX_CORRECTION 100

// Maths
#define PI 3.14159265358979323846
#define E 2.71828182845904523536
#define DEG_RAD 0.017453292519943295 // multiply to convert degrees to radians
#define RAD_DEG 57.29577951308232 // multiply to convert radians to degrees

// Camera
#define CAM_DATA_LEN 8
#define CAM_BEGIN_BYTE 0xB
#define CAM_END_BYTE 0xE
extern int16_t CAM_OFFSET_X;
extern int16_t CAM_OFFSET_Y;
#define CAM_ANGLE_OFFSET 0
#define CAM_NO_VALUE 0xBAD
#define CAM_UART_TX 17
#define CAM_UART_RX 16

// IMU
#define BNO_QUAT_FLOAT (1.0f / (1 << 14)) // converts quaternion units to floating point numbers on the BNO055
#define BNO_BUS I2C_NUM_1 // which I2C bus the BNO is on

// Orbit
extern uint8_t BALL_FAR_STRENGTH;
extern uint8_t BALL_CLOSE_STRENGTH;
extern uint8_t ORBIT_SPEED_SLOW;
extern uint8_t ORBIT_SPEED_FAST;
#define ORBIT_SlOW_ANGLE_MIN 45
#define ORBIT_SLOW_ANGLE_MAX 360 - ORBIT_SlOW_ANGLE_MIN
#define ORBIT_SLOW_STRENGTH 150
#define ORBIT_SLOW_SPEED_THING 20

extern float ORBIT_CONST;

// Attacker FSM defines
extern uint16_t DRIBBLE_BALL_TOO_FAR; // if less than this, switch out of dribble
extern uint16_t ORBIT_DIST;  // switch from orbit to pursue if value is more than this
extern uint16_t IN_FRONT_MIN_ANGLE; // angle range in which the ball is considered to be in front of the robot
extern uint16_t  IN_FRONT_MAX_ANGLE;
#define IN_FRONT_ANGLE_BUFFER 10
#define IN_FRONT_STRENGTH_BUFFER 5
#define IDLE_TIMEOUT 500 // if ball is not visible for this length of time in ms or more, switch to idle state
#define IDLE_DISTANCE 90 // distance to sit away from the goal if no ball is visible
#define IDLE_OFFSET 0
#define DRIBBLE_TIMEOUT 100 // ms, if robot sees ball in this position for this time it will switch to dribble state
#define DRIBBLE_SPEED 100 // speed at which robot dribbles the ball, out of 100
#define ACCEL_PROG 0.1 // update the acceleration interpolation by this amount per tick, 1 tick is about 10ms, so 0.01 will accelerate completely in 1 second
#define GOAL_MIN_ANGLE 30
#define GOAL_MAX_ANGLE 330
#define GOAL_SHOOT_DIST 40 // if we are within this distance, shoot

// Defence FSM defines
extern uint8_t DEFEND_DISTANCE;
extern uint8_t SURGE_DISTANCE;
extern uint8_t SURGE_STRENGTH;
#define SURGE_SPEED 100
#define REVERSE_SPEED 60
#define DEFEND_MIN_STRENGTH 70
#define DEFEND_MAX_ANGLE 120
#define DEFEND_MIN_ANGLE 250
#define KICKER_STRENGTH 100 // if ball strength greater than this, kick
#define SURGEON_ANGLE_MIN 6 // angles to surge between
#define SURGEON_ANGLE_MAX 360 - SURGEON_ANGLE_MIN
#define SURGE_CAN_KICK_TIMEOUT 500 // ms to be in surge for before we can kick

// General FSM defines
#define MODE_ATTACK 0
#define MODE_DEFEND 1
#define FSM_MAX_STATES 1024 // if there's more than this number of states in the history, it will be cleared
extern uint8_t ROBOT_MODE;

// Kicker
#define KICKER_PIN 33
#define KICKER_DELAY 10 // ms to wait between solenoid activation and deactivation
#define SHOOT_TIMEOUT 1000 // ms until we are allowed to kick again

// Buttons
#define RST_BTN 35

/** initialises per robot values */
void defines_init(uint8_t robotId);