#include <Arduino.h>
#include "Utils.h"
#include "Config.h"
#include "Move.h"
#include "LightSensorArray.h"
#include "IMU.h"
#include "Timer.h"
#include "I2C.h"
#include <i2c_t3.h>
#include "pb_decode.h"
#include "pb_encode.h"
#include "i2c.pb.h"
#include "Cam.h"
#include "Playmode.h"
#include "PID.h"

typedef enum {
    MSG_PUSH_I2C_SLAVE = 0, // as the I2C slave, I'm providing data to the I2C master
    MSG_PUSH_I2C_MASTER, // as the I2C master, I'm providing data to the I2C slave
    MSG_PULL_I2C_SLAVE, // requesting data from the I2C slave
} msg_type_t;

static I2CMasterProvide lastMasterProvide = I2CMasterProvide_init_zero;

IMU imu;
LightSensorArray ls;
Move move;
Camera cam;
Playmode playmode;

// PIDs
PID headingPID(HEADING_KP, HEADING_KI, HEADING_KD, HEADING_MAX_CORRECTION);
PID goalPID(GOAL_KP, GOAL_KI, GOAL_KD, GOAL_MAX_CORRECTION);
PID goaliePID(GOALIE_KP, GOALIE_KI, GOALIE_KD, GOALIE_MAX);

// LED Stuff
Timer lineLedTimer(200000); // LED timer when attacking
Timer movingLedTimer(500000); // LED timer when defending
Timer idleLedTimer(1000000); // LED timer when idling
Timer batteryLedTimer(10000); // LED timer when low battery
bool ledOn;

// Timeout for when ball is not visible
Timer noBallTimer(500000); // LED timer when line avoiding or surging or whatnot

// Acceleration
Timer accelTimer(ACCEL_TIME_STEP);

float targetDirection;
float targetSpeed;

// Variables which i couldn't be bothered to find a good place for
float batteryVoltage;
double heading;

float ballAngle;
bool ballVisible;

float direction;
float speed;
float orientation;

/** decode protobuf over UART from ESP **/
static void decodeProtobuf(void){
    uint8_t buf[64] = {0};
    uint8_t msg[64] = {0}; // message working space, used by nanopb
    uint8_t i = 0;

    // read in bytes from UART
    while (true){
        // force wait for more bytes to be available
        // FIXME this is a large performance bottleneck
        // we can't multi thread this (as we're on a Teensy...) but we could possibly hack up a rewrite in a way that 
        // allows the main loop to keep polling the sensors while checking for available serial bytes to append to the 
        // decode buffer. otherwise, we can just clock UART faster (perhaps even up to 1 MHz+?) which should help
        // after all we only need about 250-500 Hz
        while (!ESPSERIAL.available());
        uint8_t byte = ESPSERIAL.read();
        buf[i++] = byte;

        // terminate decoding if stream is finished
        if (byte == 0xEE){
            break;
        }    
    }

    // now we can parse the header and decode the protobuf byte stream
    if (buf[0] == 0xB){
        msg_type_t msgId = (msg_type_t) buf[1];
        uint8_t msgSize = buf[2];

        // remove the header by copying from byte 3 onwards, excluding the end byte (0xEE)
        memcpy(msg, buf + 3, msgSize);

        pb_istream_t stream = pb_istream_from_buffer(msg, msgSize);
        void *dest = NULL;
        void *msgFields = NULL;

        // assign destination struct based on message ID
        switch (msgId){
            case MSG_PUSH_I2C_MASTER:
                dest = (void*) &lastMasterProvide;
                msgFields = (void*) &I2CMasterProvide_fields;
                break;
            default:
                Serial.printf("[Comms] Unknown message ID: %d\n", msgId);
                return;
        }

        // decode the byte stream
        if (!pb_decode(&stream, (const pb_field_t *) msgFields, dest)){
            Serial.printf("[Comms] Protobuf decode error: %s\n", PB_GET_ERROR(&stream));
        } else {
            // Serial.println("Protobuf decode OK!");
        }

        // TODO do we need the backup and restore code (if there's a decode error or the packet is wack) like before?
    } else {
        Serial.printf("[Comms] Invalid begin character: 0x%X\n", buf[0]);
        delay(15);
    }
}

void setup() {
    // Put other setup stuff here
    Serial.begin(115200);
    ESPSERIAL.begin(115200);

    #if LS_ON
        // Init light sensors
        ls.init();
        ls.calibrate();
    #endif

    playmode.init();
    move.set();
    pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
    // Measure battery voltage
    batteryVoltage = get_battery_voltage();

    decodeProtobuf();

    // Update variables
    direction = lastMasterProvide.direction;
    speed = int((lastMasterProvide.speed/100) * 255);
    orientation = float((lastMasterProvide.orientation/100) * 255);
    heading = lastMasterProvide.heading;

    // Serial.printf("direction: %f, speed: %f, orientation: %f, heading: %f\n", direction, speed, orientation, heading);

    // Do line avoidance calcs
    #if LS_ON
        // Update line data
        ls.read();
        ls.calculateClusters();
        ls.calculateLine();

        playmode.updateLine((float)ls.getLineAngle(), (float)ls.getLineSize(), heading);
        playmode.calculateLineAvoidance(heading);
    #endif

    if(!playmode.onField){
        direction = playmode.getDirection();
        speed = playmode.getSpeed();
    }

    // Update motors
    move.motorCalc(direction, orientation, speed);
    move.go(false);

    #if LED_ON
        // Blinky LED stuff :D
        if(batteryVoltage < V_BAT_MIN){
            if(batteryLedTimer.timeHasPassed()){
                digitalWrite(LED_BUILTIN, ledOn);
                ledOn = !ledOn;
            }
        } else if(ls.isOnLine || ls.lineOver){
            if(lineLedTimer.timeHasPassed()){
                digitalWrite(LED_BUILTIN, ledOn);
                ledOn = !ledOn;
            }
        } else if(speed >= IDLE_MIN_SPEED){
            if(movingLedTimer.timeHasPassed()){
                digitalWrite(LED_BUILTIN, ledOn);
                ledOn = !ledOn;
            }
        } else {
            if(idleLedTimer.timeHasPassed()){
                digitalWrite(LED_BUILTIN, ledOn);
                ledOn = !ledOn;
            }
        }
    #endif
}