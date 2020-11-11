/*
 * This file is part of the Teensy 4 firmware project.
 * Copyright (c) 2019-2020 Team Omicron. All rights reserved.
 *
 * Team Omicron members: Lachlan Ellis, Tynan Jones, Ethan Lo,
 * James Talkington, Matt Young.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
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

bool LRF::checkSerial(HardwareSerial serial){
    if(serial.available() >= LRF_DATA_LENGTH){
        int firstByte = serial.read();
        if(firstByte == LRF_START_BYTE){
            if(serial.read() == LRF_START_BYTE){
                return true;
            }
            else return false;
        }
        else return false;
    }
    else return false;
}

uint16_t LRF::pollLRF(HardwareSerial serial){
    serial.read();
    serial.read();
    int highByte = serial.read();
    int lowByte = serial.read();
    return highByte << 8 | lowByte;
}

void LRF::read(){
    frontLRF = checkSerial(LRF1_SERIAL) ? pollLRF(LRF1_SERIAL) : frontLRF;
    rightLRF = checkSerial(LRF2_SERIAL) ? pollLRF(LRF2_SERIAL) : rightLRF;
    backLRF = checkSerial(LRF3_SERIAL) ? pollLRF(LRF3_SERIAL) : backLRF;
    leftLRF = checkSerial(LRF4_SERIAL) ? pollLRF(LRF4_SERIAL) : leftLRF;
}
