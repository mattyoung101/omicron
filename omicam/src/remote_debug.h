#pragma once
#include <stdint.h>
#include <string.h>

/** Creates the remote debug TCP socket and jpeg encoder **/
void remote_debug_init(uint16_t w, uint16_t h);

/** Destroys the TCP socket and jpeg encoder **/
void remote_debug_dispose();

/** Encodes a frame with libjpeg-turbo and then posts it over the TCP socket **/
void remote_debug_post_frame(uint8_t *frame, size_t frameSize);