#include <Arduino.h>
#include <Wire.h>
#include "Timer.h"

void setup() {
    #if I2C_ON
        Wire.begin(0x12);
        Wire.onRequest(requestEvent);
        Wire.onReceive(receiveEvent);
    #endif

    Serial.begin(115200);

    // TODO: mouse sensor code
}

void loop() {
    
}