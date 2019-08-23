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

typedef enum {
    MSG_PUSH_I2C_SLAVE = 0, // as the I2C slave, I'm providing data to the I2C master
    MSG_PUSH_I2C_MASTER, // as the I2C master, I'm providing data to the I2C slave
    MSG_PULL_I2C_SLAVE, // requesting data from the I2C slave
} msg_type_t;

static I2CMasterProvide lastMasterProvide = I2CMasterProvide_init_zero;

IMU imu;
LightSensorArray ls;
Move move;

// LED Stuff
Timer idleLedTimer(400000); // LED timer when idling
Timer movingLedTimer(200000); // LED timer when moving
Timer lineLedTimer(100000); // LED timer when line avoiding
Timer batteryLedTimer(50000); // LED timer when low battery
bool ledOn;

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

// Line avoidance calculation
void calcLineAvoid(){
    if(ls.isOnLine || ls.lineOver){
        if(ls.lineSize > LINE_BIG_SIZE || ls.lineSize == -1){
            if(ls.lineOver){
                targetDirection = ls.isOnLine ? doubleMod(ls.lineAngle-heading, 360) : doubleMod(ls.firstAngle-heading+180, 360);
            }else{
                targetDirection = doubleMod(ls.lineAngle-heading+180, 360);
            }
            speed = OVER_LINE_SPEED;
        }else if(ls.lineSize >= LINE_SMALL_SIZE && !ballVisible){
            if(abs(ls.firstAngle+ballAngle) < 90 && abs(ls.firstAngle+ballAngle) > 270){
                targetDirection = doubleMod(ls.firstAngle-heading+180, 360);
                speed = 0;
                // Serial.println("stopping");
            }else{
                speed = LINE_TRACK_SPEED;
            }
        }else{
            if(ls.isOnLine) speed *= LINE_SPEED_MULTIPLIER;
        }
    }
}

// Acceleration calculation
void calcAccel(){
    if(accelTimer.timeHasPassed()){ // Time step has occured, updating motor values
        // Turn target velocity vector into cartesian
        double r = targetSpeed;
        double theta = -1.0 * (targetDirection - 90);
        double targetX = r * cos(theta);
        double targetY = r * sin(theta);

        // Turn current velocity vector into cartesian
        r = speed;
        theta = -1.0 * (direction - 90);
        double currentX = r * cos(theta);
        double currentY = r * sin(theta);

        // Do accleration thingo
        double newX = currentX + sign(targetX - currentX) * ACCEL_PROGRESS;
        double newY = currentY + sign(targetY - currentY) * ACCEL_PROGRESS;

        // Convert new velocity vector to polar
        speed = sqrt(sq(newX) + sq(newY));
        direction = doubleMod(450 - (atan2(newY, newX) * RAD_DEG), 360);
    }
}

/** decode protobuf over UART from ESP **/
static void decodeProtobuf(void){
    uint8_t buf[64] = {0}; // 64 byte buffer, same as defined on the ESP
    uint8_t msg[64] = {0}; // message working space
    uint8_t i = 0;

    // read in bytes from UART
    while (true){
        uint8_t byte = ESPSERIAL.read();
        buf[i++] = byte;

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
        }

        // TODO do we need the backup and restore code (if there's a decode error or the packet is wack) like before?
    } else {
        Serial.printf("[Comms] Invalid begin character: %d\n", buf[0]);
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

    move.set();
    pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
    #if LS_ON
        // Update line data
        ls.read();
        ls.calculateClusters();
        ls.calculateLine();

        ls.updateLine((float)ls.getLineAngle(), (float)ls.getLineSize(), imu.heading);
        ls.lineCalc();
    #endif

    // Measure battery voltage
    batteryVoltage = get_battery_voltage();

    decodeProtobuf();

    // Update variables
    targetDirection = lastMasterProvide.direction;
    targetSpeed = lastMasterProvide.speed;
    orientation = lastMasterProvide.orientation;
    heading = lastMasterProvide.heading;

    // Do line avoidance calcs
    calcLineAvoid();

    // Update motors
    calcAccel();
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
    
    // Print stuffs
    // Serial.print(heading);
    // Serial.println();
}