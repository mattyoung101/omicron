#include "comms_uart.h"
#include <termios.h>
#include <fcntl.h>
#include <stdio.h>
#include <log/log.h>
#include <unistd.h>
#include <errno.h>
#include "defines.h"

// TODO we also need to have a receive task here as well
static int serialfd;
static bool uartInitOk = false;

void comms_uart_init(){
#if BUILD_TARGET == BUILD_TARGET_SBC
    // sources:
    // - https://chrisheydrick.com/2012/06/17/how-to-read-serial-data-from-an-arduino-in-linux-with-c-part-3/
    // - https://en.wikibooks.org/wiki/Serial_Programming/termios
    serialfd = open("/dev/ttyACM0", O_RDWR | O_NOCTTY); // TODO not sure if this is the right port?
    if (serialfd == -1){
        log_error("Failed to open UART bus: %s", strerror(errno));
        return;
    } else if (!isatty(serialfd)){
        log_error("UART bus claims to not be a tty!");
        return;
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

    // wait till we receive 3 byte header - TODO do we actually want to do this??? probably?
    toptions.c_cc[VMIN] = 3;

    // set the options
    tcsetattr(serialfd, TCSANOW, &toptions);
    uartInitOk = true;
    log_info("UART comms initialised successfully!");
#else
    log_warn("UART comms disabled in BUILD_TARGET_PC.");
#endif
}

void comms_uart_send(uint8_t *data, size_t size){
#if BUILD_TARGET == BUILD_TARGET_SBC
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
    log_trace("Disposing UART");
    close(serialfd);
}