#pragma once
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "protobuf/RemoteDebug.pb.h"
#include "nanopb/pb_encode.h"
#include "pb.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Creates the remote debug TCP socket and image encoder **/
void remote_debug_init(uint16_t w, uint16_t h);

/** Destroys the TCP socket and image encoder **/
void remote_debug_dispose(void);

/**
 * Encodes a frame with libjpeg-turbo and then posts it over the TCP socket
 * @param camFrame copy of original camera frame buffer, must be freed
 * @param threshFrame copy of GPU processed (thresholded) frame buffer, must be freed
 * @param ballRect the bounding box of the ball
 * @param ballCentroid the centroid of the ball
 **/
void remote_debug_post(uint8_t *camFrame, uint8_t *threshFrame, RDRect ballRect, RDPoint ballCentroid);
/** Checks if the remote debugger currently has an established connection **/
bool remote_debug_is_connected(void);

#ifdef __cplusplus
}
#endif