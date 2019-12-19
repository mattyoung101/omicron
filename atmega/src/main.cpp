#include <Arduino.h>
#include <mouse.h>

PMW3360 mouse;

void setup() {
    Serial.begin(9600);
    Serial.println("ready to pop");
    Serial.println(mouse.begin(10));
    Serial.println("aight we done");
}

void loop() {
    PMW3360_DATA data = mouse.readBurst();
    Serial.print(data.dx);
    Serial.print(", ");
    Serial.println(data.dy);
}