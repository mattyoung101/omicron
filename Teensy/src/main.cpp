#include <Arduino.h>
#include "Utils.h"
#include "Config.h"
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

// LED Stuff

Timer idleLedTimer(400000); // LED timer when idling
Timer movingLedTimer(200000); // LED timer when moving
Timer yeetLedTimer(100000); // LED timer when surging or dribbling
Timer lineLedTimer(50000); // LED timer when line avoiding
bool ledOn;

// Variables which i couldn't be bothered to find a good place for
float batteryVoltage;
double heading;

float ballAngle;
bool ballVisible;

float direction;
float speed;
float orientation;

// I2C Stuff
uint8_t dataOut[1]; // TODO
char dataIn[1];

void requestEvent(void);
void receiveEvent(size_t count);

void setup() {
    // Put other setup stuff here
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

    #if LS_ON
    // Init light sensors
    ls.init();
    ls.calibrate();
    #endif

    pinMode(LED_BUILTIN, OUTPUT);
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

    Serial.printf("  %f", ls.lineAngle);
    #endif

    if(idleLedTimer.timeHasPassed()){
        digitalWrite(LED_BUILTIN, ledOn);
        ledOn = !ledOn;
    }

    // Measure battery voltage
    batteryVoltage = get_battery_voltage();

    Serial.println();
}

void requestEvent() {
    // don't think we need this event??
}

void receiveEvent(size_t count) {
    uint8_t buf[64] = {0}; // 64 byte buffer, same as defined on the ESP
    uint8_t msg[64] = {0}; // message working space
    uint8_t i = 0;

    // read in bytes from I2C until we receive the termination character
    while (true) {
        uint8_t byte = Wire.read();
        buf[i++] = byte;

        if (byte == 0xEE){
            break;
        }
    }

    Serial.printf("Received %d bytes\n", i);

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
                Serial.printf("[I2C error] Unknown message ID: %d\n", msgId);
                return;
        }

        // decode the byte stream
        if (!pb_decode(&stream, (const pb_field_t *) msgFields, dest)){
            Serial.printf("[I2C error] Protobuf decode error: %s\n", PB_GET_ERROR(&stream));
        }
        // TODO do we need the backup and restore code (if there's a decode error or the packet is wack) like before?
    } else {
        Serial.printf("[I2C error] Invalid begin character: %d\n", buf[0]);
        delay(15);
    }
}