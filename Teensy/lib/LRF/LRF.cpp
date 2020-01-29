#include "LRF.h"
#include "Config.h"

// Datasheet thingo (it barely passes as one): http://img.banggood.com/file/products/20180830020532SKU645408.pdf

void LRF::setLRF(HardwareSerial serial){
    // Set command buffer
    sendBuf[0] = LRF_START_BYTE;
    sendBuf[1] = 0xAF;
    sendBuf[2] = 0x54;

    serial.begin(9600);
    for(int i : sendBuf) {
        serial.write(i);
    }
    serial.begin(115200);
}

void LRF::init(){
    setLRF(LRF1_SERIAL);
    setLRF(LRF2_SERIAL);
    setLRF(LRF3_SERIAL);
    setLRF(LRF4_SERIAL);
}

uint16_t LRF::pollLRF(HardwareSerial serial){
    if(serial.available() >= LRF_DATA_LENGTH){
        int firstByte = serial.read();
        if(firstByte == LRF_START_BYTE){
            if(serial.read() == LRF_START_BYTE){
                serial.read();
                serial.read();
                int highByte = serial.read();
                int lowByte = serial.read();
                return highByte << 8 | lowByte;
            }
        }
    }
}

void LRF::read(){
    frontLRF = pollLRF(LRF1_SERIAL);
    rightLRF = pollLRF(LRF2_SERIAL);
    backLRF = pollLRF(LRF3_SERIAL);
    leftLRF = pollLRF(LRF4_SERIAL);
}