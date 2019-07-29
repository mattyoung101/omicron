#include <Arduino.h>
#include "Utils.h"
#include "LightSensorArray.h"
#include "IMU.h"
#include "Timer.h"

// I would instantiate a timer class so we can blink the led but im a fucking idiot and used pin 13 for SPI cos im a fucking idiot

IMU imu;
LightSensorArray ls;

// Variables which i couldn't be bothered to find a good place for
float batteryVoltage;

void setup() {
	// Put other setup stuff here
    Serial.begin(9600);

    // join bus on address 0x12 (in slave mode)
    Wire.begin(0x12);
    Wire.onRequest(requestEvent);
    Wire.onReceive(receiveEvent);

    // Init IMU stuff
    imu.init();
    imu.calibrate();

    // Init light sensors
    ls.init();
    ls.calibrate();
}

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
    // TODO
}

void receiveEvent(int bytes) {
    // TODO
}