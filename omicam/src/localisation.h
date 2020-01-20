#pragma once
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/** An entry submitted to the localisation work queue **/
typedef struct {
    uint8_t *frame;
    int32_t width;
    int32_t height;
} localiser_entry_t;


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