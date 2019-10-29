#pragma once
#include <stdint.h>
#include <string.h>

/** Creates the remote debug TCP socket and image encoder **/
void remote_debug_init(uint16_t w, uint16_t h);
/** Destroys the TCP socket and image encoder **/
void remote_debug_dispose();
/**
 * Encodes a frame with libjpeg-turbo and then posts it over the TCP socket
 * @param camFrame copy of original camera frame buffer, must be freed
 * @param threshFrame copy of GPU processed (thresholded) frame buffer, must be freed
 **/
void remote_debug_post_frame(uint8_t *camFrame, uint8_t *threshFrame);