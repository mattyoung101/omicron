#include <Arduino.h>
#include "Utils.h"
#include "Config.h"
#include "LightSensorController.h"
#include "LRF.h"
#include "Timer.h"
#include "pb_common.h"
#include "pb_decode.h"
#include "pb_encode.h"
#include "i2c.pb.h"

// IMPORTANT: WE ARE USING UART FOR PROTOBUF NOT I2C, WE ARE JUST TOO LAZY TO RENAME STUFF

typedef enum {
    MSG_ANY = 0
} msg_type_t;

static I2CMasterProvide lastMasterProvide = I2CMasterProvide_init_zero;

LRF lrfs;
LightSensorController ls;

Vector lineAvoid = Vector(0, 0);

// LED Stuff
Timer idleLedTimer(1000000); // LED timer when idling
bool ledOn;
double heading;

/** decode protobuf over UART from ESP **/
static void decodeProtobuf(void){
    uint8_t buf[64] = {0};
    uint8_t msg[64] = {0}; // message working space, used by nanopb
    uint8_t i = 0;

    // read in bytes from UART
    while (true){
        // force wait for more bytes to be available
        // FIXME this is a large performance bottleneck
        // we can't multi thread this (as we're on a Teensy...) but we could possibly hack up a rewrite in a way that 
        // allows the main loop to keep polling the sensors while checking for available serial bytes to append to the 
        // decode buffer. otherwise, we can just clock UART faster (perhaps even up to 1 MHz+?) which should help
        // after all we only need about 250-500 Hz
        while (!ESPSERIAL.available());
        uint8_t byte = ESPSERIAL.read();
        buf[i++] = byte;

        // terminate decoding if stream is finished
        if (byte == 0xEE){
            break;
        }    
    }

    // now we can parse the header and decode the protobuf byte stream
    if (buf[0] == 0xB){
        msg_type_t msgId = (msg_type_t) buf[1];
        uint8_t msgSize = buf[2];

        // remove the header by copying from byte 3 onwards, excluding the end byte (0xEE)
        memcpy(msg, buf + 3, msgSize);

        pb_istream_t stream = pb_istream_from_buffer(msg, msgSize);
        void *dest = NULL;
        void *msgFields = NULL;

        // assign destination struct based on message ID
        switch (msgId){
            case MSG_ESP_TO_TEENSY:
                dest = (void*) &lastMasterProvide;
                msgFields = (void*) &I2CMasterProvide_fields;
                break;
            default:
                Serial.printf("[Comms] Unknown message ID: %d\n", msgId);
                return;
        }

        // decode the byte stream
        if (!pb_decode(&stream, (const pb_field_t *) msgFields, dest)){
            Serial.printf("[Comms] Protobuf decode error: %s\n", PB_GET_ERROR(&stream));
        } else {
            // Serial.println("Protobuf decode OK!");
        }

        // TODO do we need the backup and restore code (if there's a decode error or the packet is wack) like before?
    } else {
        Serial.printf("[Comms] Invalid begin character: 0x%X\n", buf[0]);
        delay(15);
    }
}

void setup() {
    // Put other setup stuff here
    Serial.begin(115200);
    ESPSERIAL.begin(115200);

    #if LS_ON
        // Init light sensors
        ls.setup();
        //ls.calibrate if request here
        ls.setThresholds();
    #endif

    #if LRFS_ON
        // Init LRFs
        lrfs.init();
    #endif    

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
}

void loop() {
    // Poll UART and decode incoming protobuf message
    decodeProtobuf();
    

    #if LS_ON
        // Update line data
        lineAvoid = ls.update(heading);

        // (ETHAN) NOTE TO SELF: RETURN LINEANGLE, LINEDISTANCE AND ISOUT
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
}