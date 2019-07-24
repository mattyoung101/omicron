#ifndef IMU_H
#define IMU_H

#include <Arduino.h>
#include <I2C.h>
#include <Utils.h>
#include <Config.h>

class IMU {
public:
    double heading;

    IMU() {};
    void init();

    Vector3D readAccelerometer();
    Vector3D readGyroscope();
    Vector3D readMagnetometer();

    void update();
    void calibrate();

private:
    long previousTimeGyro;
    double calibrationGyro;

    double convertRawAcceleration(int raw) {
        // Since we are using 2G range
        // -2g maps to a raw value of -32768
        // +2g maps to a raw value of 32767

        double a = (raw * 2.0) / 32768.0;
        return a;
    }

    double convertRawGyro(int raw) {
        // Since we are using 1000 degrees/seconds range
        // -1000 maps to a raw value of -32768
        // +1000 maps to a raw value of 32767

        double g = (raw * 1000.0) / 32768.0;
        return g;
    }
};

#endif
