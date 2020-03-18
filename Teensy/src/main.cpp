#include <Arduino.h>
#include "Utils.h"
#include "Config.h"
#include "LightSensorController.h"
#include "LRF.h"
#include "Timer.h"
#include "pb_common.h"
#include "pb_decode.h"
#include "pb_encode.h"
#include "wirecomms.pb.h"

// IMPORTANT: WE ARE USING UART FOR PROTOBUF NOT I2C, WE ARE JUST TOO LAZY TO RENAME STUFF

typedef enum {
    MSG_ANY = 0
} msg_type_t;

static MasterToLSlave lastMasterProvide = MasterToLSlave_init_zero;

LRF lrfs;
LightSensorController ls;

Vector lineAvoid = Vector(0, 0);

// LED Stuff
Timer idleLedTimer(500000); // LED timer when idling
bool ledOn;
double heading;

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

/** Decode protobuf over UART from ESP */
static void decodeProtobuf(void){
    // check if we have a syncword available
    uint8_t byte = ESPSERIAL.read();
    if (byte != 0xB){
        digitalWrite(LED_BUILTIN, LOW);
        return;
    }

    // if we do, read in the header
    msg_type_t msgId = (msg_type_t) ESPSERIAL.read();
    uint8_t msgSize = ESPSERIAL.read();
    uint8_t buf[msgSize];
    memset(buf, 0, msgSize);

    // read in the rest of the message now
    size_t readBytes = ESPSERIAL.readBytes(buf, msgSize);
    if (readBytes != msgSize){
        // if this happens a lot, it's probably timing out, look into Serial.setTimeout()
        // that's 1 second by default so likely you're going to be having more issues than
        // just that
        Serial.printf("[Comms] [ERROR] Expected %d bytes, read %d bytes\n", msgSize, readBytes);
        digitalWrite(LED_BUILTIN, LOW);
    } else {
        // Serial.printf("[Comms] [INFO] read the correct number of bytes\n");
        digitalWrite(LED_BUILTIN, HIGH);
    }
    uint8_t receivedChecksum = ESPSERIAL.read();
    uint8_t actualChecksum = crc8(buf, msgSize);

    if (receivedChecksum != actualChecksum){
        Serial.printf("[Comms] [WARN] Data integrity failed, expected 0x%.2X, got 0x%.2X\n", 
                    actualChecksum, receivedChecksum);
        // flush buffer and try again, arduino has no way to flush the input buffer so here is a delicious hack
        while (ESPSERIAL.available()) { ESPSERIAL.read(); }
        digitalWrite(LED_BUILTIN, LOW);
        return;
    } else {
        digitalWrite(LED_BUILTIN, HIGH);
        // Serial.println("[Comms] [INFO] Data integrity PASSED");
    }

    // we have only one message we receive, so don't bother with IDs
    pb_istream_t istream = pb_istream_from_buffer(buf, msgSize);
    if (!pb_decode(&istream, MasterToLSlave_fields, &lastMasterProvide)){
        Serial.printf("[Comms] [WARN] Protobuf decode error: %s\n", PB_GET_ERROR(&istream));
        digitalWrite(LED_BUILTIN, LOW);
    } else {
        Serial.printf("[Comms] [INFO] Data check passed and decode successful\n", lastMasterProvide.heading);
        digitalWrite(LED_BUILTIN, HIGH);
    }
}

void setup() {
    // Put other setup stuff here
    Serial.begin(9600);
    ESPSERIAL.begin(115200);

    #if LS_ON
        // Init light sensors
        ls.setup();
        // ls.calibrate if request here
        ls.setThresholds();
    #endif

    #if LRFS_ON
        // Init LRFs
        lrfs.init();
    #endif    

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    // ls.calibrate();

    // while(true) {
    //     Serial.println("cool done ok");
    // }

    Serial.println("INIT DONE");
}

void loop() {
    // Serial.println("  LOOP");
    // Poll UART and decode incoming protobuf message
    decodeProtobuf();

    #if LS_ON
        // Update line data
        lineAvoid = ls.update(heading);
    #endif

    #if LRFS_ON
        // Update LRFs
        lrfs.read();
    #endif

    #if LED_ON
        // if(idleLedTimer.timeHasPassed()){
        //     digitalWrite(LED_BUILTIN, ledOn);
        //     ledOn = !ledOn;
        //     Serial.println("(blink)");
        // }
    #endif

    LSlaveToMaster reply = LSlaveToMaster_init_zero;
    // ethan can you please set data here, thanks man
    // No problem
    // cheers
    reply.lineAngle = lineAvoid.arg;
    reply.lineSize = lineAvoid.mag;

    uint8_t replyBuf[64] = {0};
    pb_ostream_t ostream = pb_ostream_from_buffer(replyBuf, 64);

    if (!pb_encode(&ostream, LSlaveToMaster_fields, &reply)){
        Serial.printf("[Comms] [ERROR] Protobuf reply message encode error: %s\n", PB_GET_ERROR(&ostream));
    } else {
        // Serial.println("managed to encode reply message, going to send it");

        // encode worked, so dispatch the message over UART, same format as the ESP32 in message
        uint8_t header[3] = {0xB, MSG_ANY, (uint8_t) ostream.bytes_written};
        uint8_t checksum = crc8(replyBuf, ostream.bytes_written);

        ESPSERIAL.write(header, 3);
        ESPSERIAL.write(replyBuf, ostream.bytes_written);
        ESPSERIAL.write(checksum);
        ESPSERIAL.write(0xE);
    }
}
