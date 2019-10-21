#pragma once

#include <iniparser/dictionary.h>

/**
 * Initialises MMAL and camera
 */
void camera_manager_init(dictionary *config);
void camera_manager_dispose(void);
/** Starts camera capture (blocking) **/
void camera_manager_capture(void);