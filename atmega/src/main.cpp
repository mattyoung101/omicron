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

int dx = 0;
int dy = 0;

// LED Stuff
Timer idleLedTimer(1000000); // LED timer when idling
bool ledOn;

static uint8_t crc8(uint8_t *data, size_t len){
    uint8_t crc = 0xff;
    size_t i, j;
    for (i = 0; i < len; i++) {
        crc ^= data[i];
        for (j = 0; j < 8; j++) {
            if ((crc & 0x80) != 0)
                crc = (uint8_t)((crc << 1) ^ 0x31);
            else
                crc <<= 1;
        }
    }
    return crc;
}

void requestEvent(){
    uint8_t buf[I2C_SEND_PACKET_SIZE - 1] = {I2C_START_BYTE, highByte((int16_t) dx), lowByte((int16_t) dx),
                                         highByte((int16_t) dy), lowByte((int16_t) dy)};
    for(int i=0; i<I2C_SEND_PACKET_SIZE-1; i++){
        Wire.write(buf[i]);
    }
    Wire.write(crc8(buf, I2C_SEND_PACKET_SIZE - 1));
}

void receiveEvent(int bytes){
    if(Wire.available() >= I2C_RCV_PACKET_SIZE && Wire.read() == I2C_START_BYTE){
        uint8_t dataBuf[5] = {};
        dataBuf[0] = 0xB;
        for(int i=1; i<5; i++){
            dataBuf[i] = Wire.read();
        }

        frMotor = dataBuf[1] - 100;
        brMotor = dataBuf[2] - 100;
        blMotor = dataBuf[3] - 100;
        flMotor = dataBuf[4] - 100;

        uint8_t receivedChecksum = Wire.read();
        uint8_t actualChecksum = crc8(dataBuf, 5);

        if(receivedChecksum != actualChecksum){
            Serial.print("[COMMS] [WARN] Data integrity check failed. Expected: ");
            Serial.print(actualChecksum);
            Serial.print("Got: ");
            Serial.println(receivedChecksum);
            // Flush the buffer the dank way
            while(Wire.available()){Wire.read();}
            return;
        } else {
            // Yay CRC8 check passed
        }
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