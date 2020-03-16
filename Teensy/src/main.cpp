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
Timer idleLedTimer(1000000); // LED timer when idling
bool ledOn;
double heading;

/** Decode protobuf over UART from ESP */
static void decodeProtobuf(void){
    // if we don't have a header, don't bother decoding right now.
    // give the rest of the code some time to think
    if (ESPSERIAL.available() < 3){
        return;
    }
    
    // read in the header bytes
    uint8_t header[3] = {0};
    for (int i = 0; i < 3; i++){
        header[i] = ESPSERIAL.read();
    }

    if (header[0] == 0xB){
        msg_type_t msgId = (msg_type_t) buf[1];
        uint8_t msgSize = min(buf[2], 64);
        uint8_t buf[msgSize];
        memset(buf, 0, msgSize); // probably un-necessary, just in case

        // read in the rest of the message now
        size_t readBytes = ESPSERIAL.readBytes(buf, msgSize);
        if (readBytes != msgSize){
            // if this happens a lot, it's probably timing out, look into Serial.setTimeout()
            // that's 1 second by default so likely you're going to be having more issues than
            // just that
            Serial.printf("[Comms] [WARN] Expected %d bytes, read %d bytes\n", msgSize, readBytes);
        }

        pb_istream_t stream = pb_istream_from_buffer(buf, msgSize);
        void *dest = NULL;
        void *msgFields = NULL;

        // assign destination struct based on message ID
        switch (msgId){
            case MSG_ANY:
                dest = (void*) &lastMasterProvide;
                msgFields = (void*) &MasterToLSlave_fields;
                break;
            default:
                Serial.printf("[Comms] [WARN] Unknown message ID: %d\n", msgId);
                return;
        }

        // decode the byte stream
        if (!pb_decode(&stream, (const pb_field_t *) msgFields, dest)){
            Serial.printf("[Comms] [ERROR] Protobuf decode error: %s\n", PB_GET_ERROR(&stream));
        }
    } else {
        Serial.printf("[Comms] [WARN] Invalid begin character 0x%.2X, expected 0x0B\n", buf[0]);
        delay(15);
    }
}

void crapUart() {
    // Send line values and lrf stuff across
    Serial.println("sending shit");
    ESPSERIAL.write(0xB);
    ESPSERIAL.write(0xB);
    ESPSERIAL.write(highByte(int(lineAvoid.arg)));
    ESPSERIAL.write(lowByte(int(lineAvoid.arg)));
    ESPSERIAL.write(uint8_t(lineAvoid.mag) * 100);

    if (ESPSERIAL.available()) Serial.println("FUCKING KILL ME");

    if (ESPSERIAL.available() >= 4) {
        if (ESPSERIAL.read() == 0xB && ESPSERIAL.peek() == 0xB) {
            Serial.println("found start shit");
            ESPSERIAL.read();
            heading = word(ESPSERIAL.read(), ESPSERIAL.read());
        }
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
//    crapUart();
    

    #if LS_ON
        // Update line data
        lineAvoid = ls.update(heading);
    #endif

    #if LRFS_ON
        // Update LRFs
        lrfs.read();
    #endif

    #if LED_ON
        if(idleLedTimer.timeHasPassed()){
            digitalWrite(LED_BUILTIN, ledOn);
            ledOn = !ledOn;
        }
    #endif

    LSlaveToMaster reply = LSlaveToMaster_init_zero;
    // FIXME ethan can you please set data here, thanks man

    uint8_t replyBuf[64] = {0};
    pb_ostream_t ostream = pb_ostream_from_buffer(replyBuf, 64);

    if (!pb_encode(&ostream, LSlaveToMaster_fields, &reply)){
        Serial.printf("[Comms] [ERROR] Protobuf reply message encode error: %s\n", PB_GET_ERROR(&stream));
    } else {
        Serial.println("managed to encode reply message, going to send it");
        
        // encode worked, so dispatch the message over UART, same format as the ESP32 in message
        uint8_t header[3] = {0xB, MSG_ANY, stream.bytes_written};
        ESPSERIAL.write(header, 3);
        ESPSERIAL.write(replyBuf, stream.bytes_written);
        ESPSERIAL.write(0xEE);
    }
}
