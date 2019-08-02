#include <Arduino.h>
#include "Utils.h"
#include "Config.h"
#include "LightSensorArray.h"
#include "IMU.h"
#include "Timer.h"
#include "I2C.h"
#include <i2c_t3.h>

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

void calcLineAvoid(){
    if(ls.isOnLine || ls.lineOver){
        if(ls.lineSize > LINE_BIG_SIZE || ls.lineSize == -1){
            if(ls.lineOver){
                direction = ls.isOnLine ? doubleMod(ls.lineAngle-heading, 360) : doubleMod(ls.firstAngle-heading+180, 360);
            }else{
                direction = doubleMod(ls.lineAngle-heading+180, 360);
            }
            speed = OVER_LINE_SPEED;
        }else if(ls.lineSize >= LINE_SMALL_SIZE && !ballVisible){
            if(abs(ls.firstAngle+ballAngle) < 90 && abs(ls.firstAngle+ballAngle) > 270){
                direction = doubleMod(ls.firstAngle-heading+180, 360);
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
    // ls.read();
    // ls.calculateClusters();
    // ls.calculateLine();

    // ls.updateLine((float)ls.getLineAngle(), (float)ls.getLineSize(), imu.heading);
    // ls.lineCalc();

    digitalWrite(MUX_EN, LOW);
    digitalWrite(MUX_WR, LOW);
    digitalWrite(MUX_A0, HIGH);
    digitalWrite(MUX_A1, LOW);
    digitalWrite(MUX_A2, HIGH);
    digitalWrite(MUX_A3, HIGH);
    digitalWrite(MUX_A4, HIGH);

    Serial.print(analogRead(MUX_OUT));
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
    // TODO - Not the actual code lol
    // ESP_WIRE.write(dataOut,DATA_SIZE);
}

void receiveEvent(size_t count) {
    // TODO - Not the actual code lol
    // ESP_WIRE.read(dataIn,count);  // copy Rx data to databuf
}