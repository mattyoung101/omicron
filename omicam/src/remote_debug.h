#pragma once
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "protobuf/RemoteDebug.pb.h"
#include "nanopb/pb_encode.h"
#include "pb.h"
#include "defines.h"

extern field_objects_t selectedFieldObject;

/** Used as an easier way to pass two pointers to the thread queue (since it only takes a void*) */
typedef struct {
    uint8_t *camFrame;
    uint8_t *threshFrame;
    RDRect ballRect;
    RDPoint ballCentroid;
    int32_t fps;
} frame_entry_t;

#ifdef __cplusplus
extern "C" {
#endif

/** Creates the remote debug TCP socket and image encoder **/
void remote_debug_init(uint16_t w, uint16_t h);

/** Destroys the TCP socket and image encoder **/
void remote_debug_dispose(void);

/**
 * Sends the specified data to the work queue (to be processed asynchronously), where it is encoded (with libjpeg-turbo
 * for JPEGs and zlib for threshold masks) and sent to Omicontrol. This function returns immediately.
 * @param camFrame copy of original camera frame buffer, must be freed
 * @param threshFrame copy of GPU processed (thresholded) frame buffer, must be freed
 * @param ballRect the bounding box of the ball
 * @param ballCentroid the centroid of the ball
 * @param fps the last recorded FPS measurement of the camera
 * @param videoWidth the actual width of the video, especially if cropping is enabled
 * @param videoHeight the actual height of the video, especialy if cropping is enabled
 **/
void remote_debug_post(uint8_t *camFrame, uint8_t *threshFrame, RDRect ballRect, RDPoint ballCentroid, int32_t fps,
        int32_t videoWidth, int32_t videoHeight);
/** Checks if the remote debugger currently has an established connection **/
bool remote_debug_is_connected(void);

#ifdef __cplusplus
}
#endif