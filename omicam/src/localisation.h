#pragma once
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include "protobuf/RemoteDebug.pb.h"
#include "defines.h"

/** An entry submitted to the localisation work queue */
typedef struct {
    uint8_t *frame;
    int32_t width;
    int32_t height;
} localiser_entry_t;

/** true if the localisation for the last frame has completed */
extern _Atomic bool localiserDone;
// these need to be declared extern so they can be acessed by the remote debugger
/** rays that we observed from raycasting on the camera image, no dewarping */
extern double observedRaysRaw[LOCALISER_NUM_RAYS];
/** rays that we observed from raycasting on the camera, with dewarping **/
extern double observedRays[LOCALISER_NUM_RAYS];
/** rays that we got from raycasting on the field file */
extern double expectedRays[LOCALISER_NUM_RAYS];

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialises the localiser using the provided field file
 * @param fieldFile the full path including extension to the field file
 **/
void localiser_init(char *fieldFile);

/**
 * Posts a frame to the localiser for processing. This will be sent to a work queue
 * and processed asynchronously so the function returns immediately.
 */
void localiser_post(uint8_t *frame, int32_t width, int32_t height);

/** Destroys the localiser and its resources **/
void localiser_dispose(void);

/**
 * Puts the current line point list into a Protobuf compatible format. Only for use by the remote_debug.c.
 * IMPORTANT: the localiser MUST have finished working by this point, please wait on localiserDone if unsure.
 * @param array the array to store the line points in
 * @param arraySize the max size this array can hold
 * @param dewarped if true returns dewarped and rotated points otherwise returns raw camera points
 * @return the real size of elements put into the array
 */
uint32_t localiser_remote_get_points(RDPoint *array, size_t arraySize, bool dewarped);

#ifdef __cplusplus
};
#endif