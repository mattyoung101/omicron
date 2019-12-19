#include <Arduino.h>
#include <Wire.h>
#include "Timer.h"
#include "Mouse.h"

PMW3360 mouse;

void setup() {
    #if I2C_ON
        Wire.begin(0x12);
        Wire.onRequest(requestEvent);
        Wire.onReceive(receiveEvent);
    #endif

    #if MOUSE_ON
        Serial.println(mouse.begin(10));
    #endif

    Serial.begin(115200);

    // TODO: mouse sensor code
}

void loop() {
    PMW3360_DATA data = mouse.readBurst();
    Serial.print(data.dx);
    Serial.print(", ");
    Serial.println(data.dy);
}