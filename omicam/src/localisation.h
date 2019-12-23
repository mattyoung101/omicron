#pragma once
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/**
 * Initialises the localiser using the provided field file
 * @param fieldFile the name of the field file (no path, just the name with extension)
 **/
void localiser_init(char *fieldFile);
/**
 * Posts a frame to the localiser for processing. This will be sent to a work queue
 * and processed asynchronously so the function returns immediately.
 */
void localiser_post(uint8_t *frame, uint16_t width, uint16_t height);
/** Destroys the localiser and its resources **/
void localiser_dispose(void);

extern float estimatedX;
extern float estimatedY;