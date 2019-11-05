#pragma once

#include <stdint.h>
#include <GLES2/gl2.h>
#include "bcm_host.h"
#include "interface/vcos/vcos.h"
#include "interface/mmal/mmal.h"
#include "interface/mmal/mmal_buffer.h"
#include "interface/mmal/util/mmal_util.h"
#include "interface/mmal/util/mmal_util_params.h"
#include "interface/mmal/util/mmal_default_components.h"
#include "interface/mmal/util/mmal_connection.h"

/**
 * Creates a new OpenGL context, initialises shaders, textures, etc
 * @param width the width of the framebuffer
 * @param height the height of the framebuffer
 **/
void gpu_manager_init(uint16_t width, uint16_t height);
/**
 * Posts a frame to the GPU for processing
 * @param buf the MMAL buffer header containing the frame data
 * @return The buffer containing the received pixels from the GPU. You MUST free this buffer to prevent memory leaks.
 **/
uint8_t *gpu_manager_post(MMAL_BUFFER_HEADER_T *buf);
/** Destroys the display and context safely **/
void gpu_manager_dispose(void);