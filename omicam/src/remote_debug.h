/*
 * This file is part of the Omicam project.
 * Copyright (c) 2019-2020 Team Omicron. All rights reserved.
 *
 * Team Omicron members: Lachlan Ellis, Tynan Jones, Ethan Lo,
 * James Talkington, Matt Young.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "protobuf/RemoteDebug.pb.h"
#include "nanopb/pb_encode.h"
#include "pb.h"
#include "defines.h"
#include "protobuf/UART.pb.h"

extern field_objects_t selectedFieldObject;

extern _Atomic double cpuTemperature;

/** Used as an easier way to pass two pointers to the thread queue (since it only takes a void*) */
typedef struct {
    uint8_t *camFrame;
    uint8_t *threshFrame;
    RDRect ballRect;
    RDPointF ballCentroid;
    int32_t fps;
    ObjectData objectData;
} frame_entry_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Creates the remote debugger and its resources (TCP socket, JPEG encoder, etc).
 * @param w width of the frames
 * @param h height of the frames
 */
void remote_debug_init(uint16_t w, uint16_t h);
/** Destroys the remote debugger and its resources */
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
 * @param objectData copy of the data sent over UART to the ESP32, used mainly for ball and goal positions for Omicontrol
 */
void remote_debug_post(uint8_t *camFrame, uint8_t *threshFrame, RDRect ballRect, RDPointF ballCentroid, int32_t fps,
        int32_t videoWidth, int32_t videoHeight, ObjectData objectData);
/** Checks if the remote debugger currently has an established connection */
bool remote_debug_is_connected(void);
/** Asks the localiser to provide info into the provided DebugFrame struct */
void remote_debug_localiser_provide(DebugFrame *debugFrame);

#ifdef __cplusplus
}
#endif