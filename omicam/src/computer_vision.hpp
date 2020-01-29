#pragma once

#ifdef __cplusplus
extern "C" {
#endif
/**
 * Starts OpenCV frame grabbing and processing. This blocks the current thread until capture is stopped with
 * @ref vision_dispose
 */
void vision_init(void);

/**
 * Stops capture and frees allocated OpenCV resources. This will unblock the current thread.
 */
void vision_dispose(void);
#ifdef __cplusplus
}
#endif