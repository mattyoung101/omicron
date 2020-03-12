#include <Arduino.h>
#include <Wire.h>
#include "Mouse.h"
#include "Motor.h"
#include "Config.h"
#include "Timer.h"

PMW3360 mouse;
Motor motor;

int frMotor = 0;
int brMotor = 0;
int blMotor = 0;
int flMotor = 0;

int dx;
int dy;

// LED Stuff
Timer idleLedTimer(1000000); // LED timer when idling
bool ledOn;

void requestEvent(){
    // Serial.println("BITCH");
    Wire.write(I2C_START_BYTE);
    Wire.write(highByte((int16_t) dx));
    Wire.write(lowByte((int16_t) dx));
    Wire.write(highByte((int16_t) dy));
    Wire.write(lowByte((int16_t) dy));
}

void receiveEvent(int bytes){
    // Serial.println("ASS");
    if(Wire.available() >= I2C_PACKET_SIZE && Wire.read() == I2C_START_BYTE){
        frMotor = Wire.read() - 100;
        brMotor = Wire.read() - 100;
        blMotor = Wire.read() - 100;
        flMotor = Wire.read() - 100;
    }
}

void setup(){
    Serial.begin(9600);

    // #if I2C_ON
        Wire.begin(I2C_ADDRESS);
        Wire.onRequest(requestEvent);
        Wire.onReceive(receiveEvent);
    // #endif
    
    #if MOUSE_ON
        // Serial.println(mouse.begin(10)); // I think 10 is the SS (CS) pin
    #endif

    #if LED_ON
        pinMode(13, OUTPUT);
    #endif

    motor.init();
}

void loop(){
    // #if MOUSE_ON
    //     PMW3360_DATA data = mouse.readBurst();
    //     Serial.print(data.dx);
    //     Serial.print("\t");
    //     Serial.println(data.dy);
    // #endif

    #if I2C_ON
        // dx = data.dx;
        // dy = data.dy;
    #endif

    #if LED_ON
        if(idleLedTimer.timeHasPassed()){
            digitalWrite(LED_BUILTIN, ledOn);
            ledOn = !ledOn;
        }
    #endif

    Serial.println(frMotor);

    motor.move(frMotor, brMotor, blMotor, flMotor);
}