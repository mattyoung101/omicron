#pragma once
#include <stdint.h>
#include <string.h>

/** entry in the frames to be processed queue **/
typedef struct {
    /** the frame direct from the camera **/
    uint8_t *cameraFrame;
    /** the frame processed on the GPU **/
    uint8_t *threshFrame;
} frame_entry_t;

/** A frame and its size **/
typedef struct {
    uint8_t *buf;
    unsigned long size;
} frame_t;

/** Creates the remote debug TCP socket and image encoder **/
void remote_debug_init(uint16_t w, uint16_t h);
/** Destroys the TCP socket and image encoder **/
void remote_debug_dispose();
/**
 * Encodes a frame with libjpeg-turbo or lodepng and then posts it over the TCP socket
 * @param cameraFrame the copied camera frame, must be freed
 * @param threshFrame a reference to the GPU out buffer, must be freed
 **/
void remote_debug_post_frame(uint8_t *cameraFrame, uint8_t *threshFrame);