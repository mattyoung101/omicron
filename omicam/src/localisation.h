#pragma once
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include "protobuf/RemoteDebug.pb.h"
#include "defines.h"
#include "DG_dynarr.h"

/** An entry submitted to the localisation work queue */
typedef struct {
    uint8_t *frame;
    int32_t width;
    int32_t height;
} localiser_entry_t;

typedef struct {
    double x, y;
} localiser_point_t;

/** localiser point list type */
DA_TYPEDEF(localiser_point_t, lp_list_t)

/** true if the localisation for the last frame has completed */
extern _Atomic bool localiserDone;

// these need to be declared extern so they can be acessed by the remote debugger
/** rays that we observed from raycasting on the camera image, no dewarping */
extern double observedRaysRaw[LOCALISER_NUM_RAYS];
/** rays that we observed from raycasting on the camera, with dewarping */
extern double observedRays[LOCALISER_NUM_RAYS];
/** rays that we got from raycasting on the field file */
extern double expectedRays[LOCALISER_NUM_RAYS];
/** points visited by the Subplex optimiser */
extern lp_list_t localiserVisitedPoints;
/** score for each ray in the last localiser pass */
extern double rayScores[LOCALISER_NUM_RAYS];
/** NLopt status for remote debug */
extern char localiserStatus[32];
/** localiser rate in Hz and nubmer of evals */
extern int32_t localiserRate, localiserEvals;
/** was used to try and fix a bug, not very helpful anymore */
extern pthread_mutex_t localiserMutex;

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

#ifdef __cplusplus
};
#endif