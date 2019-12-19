#ifndef LIGHT_SENSOR_ARRAY_H
#define LIGHT_SENSOR_ARRAY_H

#include <Arduino.h>
#include "Timer.h"
#include "Utils.h"
#include "Config.h"
#include "Pinlist.h"
    
// Array of light sensors
class LightSensorArray {
public:
    LightSensorArray() {}

    void init();

    void read();
    int readSensor(int sensor);
    void changeMUXChannel(uint8_t channel);

    void calculateClusters(bool doneFillInSensors = false);
    void fillInSensors();
    void calculateLine();
    void resetStartEnds();

    void calibrate();

    double getLineAngle();
    double getLineSize();

    void updateLine(float lineDir, float lineSize, float heading); // Function for updating variables
    void lineCalc(); // Function for line tracking

    bool isOnLine = false;
    bool lineOver = false;
    float lineAngle = NO_LINE_ANGLE;
    float lineSize = NO_LINE_SIZE;
    float firstAngle = NO_LINE_ANGLE;

    bool data[LS_NUM]; // Array of if sensors see white or not
    bool filledInData[LS_NUM]; // Data after sensors are filled in (if an off sensor has two adjacent on sensors, it will be turned on)

    uint16_t thresholds[LS_NUM]; // Thresholds for each sensor. A sensor is on if reading > threshold

    int starts[4]; // Array of cluster start indexes
    int ends[4]; // Array of cluster end indexes

    int numClusters = 0; // Number of clusters found

private:
    void resetClusters();

    // Index = LS num, value = mux binary
    uint8_t muxLUT[LS_NUM] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};
    bool onField;
    double fieldLineAngle = -1;
    double fieldLineSize = -1; 
    bool noLine = false;
    double angle; // Line angle
    double size; // Line size
};

#endif // LIGHT_SENSOR_ARRAY_H
