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
#ifndef LRF_H
#define LRF_H

#include "Arduino.h"
#include "Config.h"

class LRF {
    public:
        uint16_t frontLRF;
        uint16_t rightLRF;
        uint16_t backLRF;
        uint16_t leftLRF;

        void init();
        void read();
    private:
        int receiveBuf[LRF_DATA_LENGTH];
        int sendBuf[3];

        void setLRF(HardwareSerial serial);
        uint16_t pollLRF(HardwareSerial serial);
        bool checkSerial(HardwareSerial serial);
};

#endif
