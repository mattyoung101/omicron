#include <Arduino.h>
#include "Utils.h"
#include "Config.h"
#include "LightSensorArray.h"
#include "IMU.h"
#include "Timer.h"
#include <i2c_t3.h>

// I would instantiate a timer class so we can blink the led but im a fucking idiot and used pin 13 for SPI cos im a fucking idiot

IMU imu;
LightSensorArray ls;

// Variables which i couldn't be bothered to find a good place for
float batteryVoltage;

// I2C Stuff
uint8_t dataOut[1]; // TODO
char dataIn[1];

void requestEvent(void);
void receiveEvent(size_t count);

void setup() {
    // Put other setup stuff here
    Serial.begin(9600);

    // join bus on address 0x12 (in slave mode)
    ESP_WIRE.begin(0x12);
    ESP_WIRE.onRequest(requestEvent);
    ESP_WIRE.onReceive(receiveEvent);

    // Init IMU stuff
    imu.init();
    imu.calibrate();

    // Init light sensors
    ls.init();
    ls.calibrate();
}

uint16_t i = 0;
void loop() {
    // Read imu
    imu.update();

    // Update line data
    ls.read();
    ls.calculateClusters();
    ls.calculateLine();

    ls.updateLine((float)ls.getLineAngle(), (float)ls.getLineSize(), imu.heading);
    ls.lineCalc();

    // Measure battery voltage
    batteryVoltage = get_battery_voltage();
}

void requestEvent() {
    // TODO - Not the actual code lol
    // ESP_WIRE.write(dataOut,DATA_SIZE);
}

void receiveEvent(size_t count) {
    // TODO - Not the actual code lol
    // ESP_WIRE.read(dataIn,count);  // copy Rx data to databuf
}