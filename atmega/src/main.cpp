#include <Arduino.h>
#include <Wire.h>
#include "Timer.h"
#include "Mouse.h"
#include "Motor.h"
#include "Config.h"

PMW3360 mouse;
Motor motor;

int frMotor = 0;
int brMotor = 0;
int blMotor = 0;
int flMotor = 0;

int dx;
int dy;

void requestEvent(){
    Wire.write(I2C_START_BYTE);
    Wire.write(highByte((uint16_t) dx));
    Wire.write(lowByte((uint16_t) dx));
    Wire.write(highByte((uint16_t) dy));
    Wire.write(lowByte((uint16_t) dy));
}

void receiveEvent(int bytes){
    if(Wire.available() >= I2C_PACKET_SIZE && Wire.read() == I2C_START_BYTE){
        frMotor = Wire.read();
        brMotor = Wire.read();
        blMotor = Wire.read();
        flMotor = Wire.read();
    }
}

void setup(){
    Serial.begin(115200);

    #if I2C_ON
        Wire.begin(I2C_ADDRESS);
        Wire.onRequest(requestEvent);
        Wire.onReceive(receiveEvent);
    #endif
    
    #if MOUSE_ON
        Serial.println(mouse.begin(10));
    #endif

    motor.init();
}

void loop(){
    PMW3360_DATA data = mouse.readBurst();
    dx = data.dx;
    dy = data.dy;

    motor.move(frMotor, brMotor, blMotor, flMotor);
}