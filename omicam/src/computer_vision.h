#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Starts OpenCV frame grabbing and processing
 */
void vision_init(void);

/**
 * Stops capture and frees allocated OpenCV resources
 */
void vision_dispose(void);

#ifdef __cplusplus
}
#endif