#include "comms_uart.h"
#include <termios.h>
#include <fcntl.h>
#include <stdio.h>
#include <log/log.h>
#include <unistd.h>
#include <errno.h>

// Source: https://en.wikibooks.org/wiki/Serial_Programming/termios

static int serialfd;

void comms_uart_init(){
    serialfd = open("/dev/ttyACM0", O_RDWR | O_NOCTTY | O_NDELAY);
    if (serialfd == -1){
        log_error("Failed to open UART bus: %s", strerror(errno));
        return;
    } else if (!isatty(serialfd)){
        log_error("UART bus claims to not be a tty!");
        return;
    }

    struct termios config;
    tcgetattr(serialfd, &config);
}

void comms_uart_send(uint8_t *data, size_t size){
    // TODO need to package this with the ESP32 header
//    ssize_t bytesWritten = write(serialfd, data, size);
//    if (bytesWritten == -1){
//        log_error("Failed to write to UART bus: %s", strerror(errno));
//    }
}

void comms_uart_dispose(){
    log_trace("Disposing UART");
    close(serialfd);
}