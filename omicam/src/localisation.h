#pragma once
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include "protobuf/RemoteDebug.pb.h"
#include "protobuf/UART.pb.h"
#include "defines.h"
#include "DG_dynarr.h"
#include "mathc.h"

/** An entry submitted to the localisation work queue */
typedef struct {
    uint8_t *frame;
    int32_t width;
    int32_t height;
    /** goal yellow relative to robot in centimetre cartesian coordinates */
    struct vec2 yellowRel;
    /** goal blue relative to robot in centimetre cartesian coordinates */
    struct vec2 blueRel;
    bool yellowVisible;
    bool blueVisible;
} localiser_entry_t;

typedef struct {
    double x, y;
} localiser_point_t;

/** localiser point list type */
DA_TYPEDEF(localiser_point_t, lp_list_t)

/** true if the localisation for the last frame has completed */
extern _Atomic bool localiserDone;
/** was used to try and fix a bug, not very helpful anymore */
extern pthread_mutex_t localiserMutex;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialises the localiser using the provided field file
 * @param fieldFile the full path including extension to the field file
 */
void localiser_init(char *fieldFile);
/**
 * Posts a frame to the localiser for processing. This will be sent to a work queue and processed asynchronously so the
 * function returns immediately.
 * @param frame 1 bit mask of line pixels, allocated previously
 * @param width the width of the frame
 * @param height the height of the frame
 * @param yellowRel goal yellow relative to robot in centimetre cartesian coordinates
 * @param blueRel goal blue relative to robot in centimetre cartesian coordinates
 * @param yellowVisible true if yellow goal is visible in the camera
 * @param blueVisible true if blue goal is visible in the camera
 */
void localiser_post(uint8_t *frame, int32_t width, int32_t height, struct vec2 yellowRel, struct vec2 blueRel,
        bool yellowVisible, bool blueVisible);
/** Destroys the localiser and its resources */
void localiser_dispose(void);

#ifdef __cplusplus
};
#endif