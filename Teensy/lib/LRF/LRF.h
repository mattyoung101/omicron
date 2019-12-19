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
};

#endif