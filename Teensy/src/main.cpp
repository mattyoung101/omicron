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

#if ESP_I2C_ON
typedef enum {
    MSG_PUSH_I2C_SLAVE = 0, // as the I2C slave, I'm providing data to the I2C master
    MSG_PUSH_I2C_MASTER, // as the I2C master, I'm providing data to the I2C slave
    MSG_PULL_I2C_SLAVE, // requesting data from the I2C slave
} msg_type_t;

static I2CMasterProvide lastMasterProvide = I2CMasterProvide_init_zero;
#endif

IMU imu;
LightSensorArray ls;
Move move;
Camera cam;
Playmode playmode;

// PIDs
PID headingPID(HEADING_KP, HEADING_KI, HEADING_KD, HEADING_MAX_CORRECTION);
PID goalPID(GOAL_KP, GOAL_KI, GOAL_KD, HEADING_MAX_CORRECTION);
PID goaliePID(GOALIE_KP, GOALIE_KI, GOALIE_KD, GOALIE_MAX);

// LED Stuff
Timer attackLedTimer(400000); // LED timer when idling
Timer defendLedTimer(200000); // LED timer when moving
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

void requestEvent(void);
void receiveEvent(size_t count);

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

void setup() {
    // Put other setup stuff here
    delay(200); // Delay so that turning robot on doesn't yeet imu calibrations

    Serial.begin(115200);

    #if ESP_I2C_ON
    // Init ESP_WIRE
    // join bus on address 0x12 (in slave mode)
    ESP_WIRE.begin(0x12);
    ESP_WIRE.onRequest(requestEvent);
    ESP_WIRE.onReceive(receiveEvent);
    #endif


    #if MPU_I2C_ON
    // Init IMU_WIRE
    I2Cinit();

    // Init IMU stuff
    imu.init();
    imu.calibrate();
    #endif

    
    #if CAM_ON
    cam.setup();
    #endif


    #if LS_ON
    // Init light sensors
    ls.init();
    ls.calibrate();
    #endif

    move.set();

    pinMode(LED_BUILTIN, OUTPUT);
    // digitalWrite(LED_BUILTIN, HIGH);
}

void loop() {
    #if MPU_I2C_ON
    // Read imu
    imu.update();
    #endif


    #if LS_ON
    // Update line data
    ls.read();
    ls.calculateClusters();
    ls.calculateLine();

    ls.updateLine((float)ls.getLineAngle(), (float)ls.getLineSize(), imu.heading);
    ls.lineCalc();
    #endif


    #if CAM_ON
    cam.read();
    cam.calc();
    #endif


    // Measure battery voltage
    batteryVoltage = get_battery_voltage();


    // Update variables
    playmode.updateBall(cam.orange.angle, cam.orange.length, cam.orange.exists);

    #if LS_ON 
    playmode.updateLine(ls.lineAngle, ls.lineSize, imu.heading); 
    #endif
    
    if(DEFENCE){
        ENEMY_GOAL ? playmode.updateGoal(cam.yellow.angle, cam.yellow.length, cam.yellow.exists) : playmode.updateGoal(cam.blue.angle, cam.blue.length, cam.blue.exists);
    } else {
        ENEMY_GOAL ? playmode.updateGoal(cam.blue.angle, cam.blue.length, cam.blue.exists) : playmode.updateGoal(cam.yellow.angle, cam.yellow.length, cam.yellow.exists);
    }

    playmode.calculateOrbit();

    // if(playmode.getGoalVisibility() && playmode.getBallExist() && (playmode.getBallAngle() < 90 || playmode.getBallAngle() > 270) && !playmode.lineAvoiding()){
    //     if(DEFENCE){
    //         orientation = ((int)round(-goaliePID.update(doubleMod(doubleMod(playmode.getGoalAngle(), 360) + 180, 360) - 180, 0)) % 360);
    //         orientation = (int)round(headingPID.update(doubleMod(doubleMod(imu.heading, 360) + 180, 360) - 180, 0)) % 360;
    //     }
    //     else orientation = ((int)round(-goalPID.update(doubleMod(doubleMod(playmode.getGoalAngle(), 360) + 180, 360) - 180, 0)) % 360);
    // }
    // else{
        orientation = (int)round(headingPID.update(doubleMod(doubleMod(imu.heading, 360) + 180, 360) - 180, 0)) % 360;
    // }

    // Update motors
    direction = playmode.getDirection();
    speed = playmode.getSpeed();

    // calcAccel();
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
    } else {
        #if !DEFENCE
        if(attackLedTimer.timeHasPassed()){
            digitalWrite(LED_BUILTIN, ledOn);
            ledOn = !ledOn;
        }
        #else
        if(defendLedTimer.timeHasPassed()){
            digitalWrite(LED_BUILTIN, ledOn);
            ledOn = !ledOn;
        }
        #endif
    }
    #endif

    
    // Print stuffs
    Serial.printf("BallData - angle: %d, strength: %d, exists: %d", playmode.getBallAngle(), playmode.getBallDistance(), playmode.getBallExist());
    // Serial.printf("MoveData - direction: %d, orientation: %d, speed: %d", cam.orange.x, cam.orange.y, cam.orange.exists);
    // Serial.print(imu.heading);
    Serial.println();
}

#if ESP_I2C_ON
void requestEvent() {
    Serial.println("Sending heading bytes");
    // Send heading to ESP
    ESP_WIRE.write(0xB); // The :b:est start byte in existence
    ESP_WIRE.write(highByte((uint16_t) (imu.heading * 100))); // Send most sigificant byte
    ESP_WIRE.write(lowByte((uint16_t) (imu.heading * 100))); // Send least significant byte
}

void receiveEvent(size_t count) {
    // uint8_t buf[64] = {0}; // 64 byte buffer, same as defined on the ESP
    // uint8_t msg[64] = {0}; // message working space
    // uint8_t i = 0;

    // Serial.printf("receiveEvent(%d)\n", count);

    // // read in bytes from I2C until we receive the termination character
    // while (ESP_WIRE.available()) {
    //     uint8_t byte = ESP_WIRE.read();
    //     buf[i++] = byte;

    //     if (byte == 0xEE){
    //         break;
    //     }
    // }

    // Serial.printf("Received %d bytes\n", i);

    // // now we can parse the header and decode the protobuf byte stream
    // if (buf[0] == 0xB){
    //     msg_type_t msgId = (msg_type_t) buf[1];
    //     uint8_t msgSize = buf[2];

    //     // remove the header by copying from byte 3 onwards, excluding the end byte (0xEE)
    //     memcpy(msg, buf + 3, msgSize);

    //     pb_istream_t stream = pb_istream_from_buffer(msg, msgSize);
    //     void *dest = NULL;
    //     void *msgFields = NULL;

    //     // assign destination struct based on message ID
    //     switch (msgId){
    //         case MSG_PUSH_I2C_MASTER:
    //             dest = (void*) &lastMasterProvide;
    //             msgFields = (void*) &I2CMasterProvide_fields;
    //             break;
    //         default:
    //             Serial.printf("[I2C error] Unknown message ID: %d\n", msgId);
    //             return;
    //     }

    //     // decode the byte stream
    //     if (!pb_decode(&stream, (const pb_field_t *) msgFields, dest)){
    //         Serial.printf("[I2C error] Protobuf decode error: %s\n", PB_GET_ERROR(&stream));
    //     }
    //     // TODO do we need the backup and restore code (if there's a decode error or the packet is wack) like before?
    // } else {
    //     Serial.printf("[I2C error] Invalid begin character: %d\n", buf[0]);
    //     delay(15);
    // }
}
#endif