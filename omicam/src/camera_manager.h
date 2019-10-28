#pragma once

#include <iniparser/dictionary.h>

/** Initialises MMAL and camera. Also initialises the GPU manager and remote debug manager. */
void camera_manager_init(dictionary *config);
/** Destroys and disposes MMAL and the camera **/
void camera_manager_dispose(void);
/** Starts camera capture (blocking) **/
void camera_manager_capture(void);