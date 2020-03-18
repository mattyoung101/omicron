#include "comms_uart.h"
#include <termios.h>
#include <fcntl.h>
#include <stdio.h>
#include <log/log.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <nanopb/pb_decode.h>
#include "defines.h"
#include "utils.h"

#define UART_BUS_NAME "/dev/ttyACM0"

static int serialfd;
static bool uartInitOk = false;
static pthread_t receiveThread;
SensorData lastSensorData = {0};

/** UART receive task, based on the esp32 firmware comms_uart.c code */
static void *receive_thread(void *arg){
#if BUILD_TARGET == BUILD_TARGET_SBC || UART_OVERRIDE
    log_debug("UART receive task running");

    while (true){
        pthread_testcancel();

        // check if we have a begin byte available
        uint8_t byte = 0;
        read(serialfd, &byte, 1);
        if (byte != 0xB){
            continue;
        }

        // read in the rest of the header: message id and size
        uint8_t header[2] = {0};
        read(serialfd, header, 2);
        comms_msg_type_t msgType = header[0];
        uint8_t msgSize = header[1];

        // read in the rest of the data
        uint8_t data[msgSize];
        read(serialfd, data, msgSize);

        // read in and verify checksum, if it fails flush the entire buffer for this device and try again
        uint8_t receivedChecksum = 0;
        uint8_t actualChecksum = crc8(data, msgSize);
        read(serialfd, &receivedChecksum, 1);

        if (receivedChecksum != actualChecksum){
            log_warn("Data integrity check failed, expected 0x%.2X, got 0x%.2X", actualChecksum, receivedChecksum);
            tcflush(serialfd, TCIFLUSH);
            continue;
        } else {
             log_trace("Data integrity check passed!");
        }

        pb_istream_t stream = pb_istream_from_buffer(data, msgSize);
        // currently the only message we receive from the esp32 is the sensor data message, so we can just set the
        // dest struct and fields directly
        if (!pb_decode(&stream, SensorData_fields, &lastSensorData)){
            log_error("Failed to decode ESP32 Protobuf message: %s", PB_GET_ERROR(&stream));
        } else {
            log_trace("Decoded UART message successfully!! Yay, this means UART works everywhere. Nice.");
        }

        pthread_testcancel();
    }
#else
    return NULL;
#endif
}

void comms_uart_init(){
#if BUILD_TARGET == BUILD_TARGET_SBC || UART_OVERRIDE
    // sources:
    // - https://chrisheydrick.com/2012/06/17/how-to-read-serial-data-from-an-arduino-in-linux-with-c-part-3/
    // - https://en.wikibooks.org/wiki/Serial_Programming/termios
    log_debug("Opening UART bus: " UART_BUS_NAME);
    serialfd = open(UART_BUS_NAME, O_RDWR | O_NOCTTY);
    if (serialfd == -1){
        log_error("Failed to open UART bus: %s", strerror(errno));
        goto die;
    } else if (!isatty(serialfd)){
        log_error("UART bus claims to not be a tty!");
        goto die;
    }

    // get current config
    struct termios toptions;
    tcgetattr(serialfd, &toptions);

    // set 115200 baud
    cfsetspeed(&toptions, B115200);

    // &= followed by ~ means toggle off, |= means toggle on
    // 8 bits, no parity bit, 1 stop bit
    toptions.c_cflag &= ~PARENB;
    // TODO CSTOPB will either need to be set here with |=, commented out or unset like it is now, depending on what works
    toptions.c_cflag &= ~CSTOPB;
    toptions.c_cflag &= ~CSIZE;
    toptions.c_cflag |= CS8;

    // no hardware flow control
    toptions.c_cflag &= ~CRTSCTS;
    // enable receiver, ignore status lines
    toptions.c_cflag |= CREAD | CLOCAL;
    // disable input/output flow control, disable restart chars
    toptions.c_iflag &= ~(IXON | IXOFF | IXANY);
    // disable canonical input, disable echo, disable visually erase chars, disable terminal-generated signals
    toptions.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    // disable output processing
    toptions.c_oflag &= ~OPOST;

    // wait till we receive 3 byte header
    toptions.c_cc[VMIN] = 3;
    // wait for 0.25 seconds (yes it's actually specified in this way as tenths of a second... ugh)
    toptions.c_cc[VTIME] = 025;

    // set the options
    tcsetattr(serialfd, TCSANOW, &toptions);
    uartInitOk = true;

    // start the receive task
    int err = pthread_create(&receiveThread, NULL, receive_thread, NULL);
    if (err != 0){
        log_error("Failed to create UART receive thread: %s", strerror(err));
    } else {
        pthread_setname_np(receiveThread, "UART Receive");
    }

    log_info("UART comms initialised successfully!");
    return;

    die:
        // if we can't open the UART bus while on the SBC, we're essentially useless
        log_error("Critical failure: cannot communicate to ESP32 in this state. Impossible to continue running Omicam!");
        exit(EXIT_FAILURE);
#else
    log_warn("UART comms disabled in BUILD_TARGET_PC.");
#endif
}

void comms_uart_send(uint8_t *data, size_t size){
#if BUILD_TARGET == BUILD_TARGET_SBC || UART_OVERRIDE
    if (!uartInitOk){
        log_error("Cannot write to UART because UART did not initialise correctly!");
        return;
    }
    ssize_t bytesWritten = write(serialfd, data, size);
    if (bytesWritten == -1){
         log_error("Failed to write to UART bus: %s", strerror(errno));
    }
#endif
}

void comms_uart_dispose(){
#if BUILD_TARGET == BUILD_TARGET_SBC || UART_OVERRIDE
    log_trace("Disposing UART");
    close(serialfd);
    pthread_cancel(receiveThread);
#endif
}