#include <Arduino.h>
#include <Wire.h>
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
    Wire.write(highByte((int16_t) dx));
    Wire.write(lowByte((int16_t) dx));
    Wire.write(highByte((int16_t) dy));
    Wire.write(lowByte((int16_t) dy));
}

void receiveEvent(int bytes){
    if(Wire.available() >= I2C_PACKET_SIZE && Wire.read() == I2C_START_BYTE){
        frMotor = word(Wire.read(), Wire.read());
        brMotor = word(Wire.read(), Wire.read());
        blMotor = word(Wire.read(), Wire.read());
        flMotor = word(Wire.read(), Wire.read());
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
        Serial.println(mouse.begin(10)); // I think 10 is the SS (CS) pin
    #endif

    motor.init();
}

void loop(){
    PMW3360_DATA data = mouse.readBurst();
    dx = data.dx;
    dy = data.dy;

    motor.move(frMotor, brMotor, blMotor, flMotor);
}