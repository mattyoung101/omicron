#pragma once

#include <stdint.h>

/**
 * Creates a new OpenGL context and initialises the VideoCore
 * @param width the width of the framebuffer
 * @param height the height of the framebuffer
 **/
void gpu_manager_init(uint16_t width, uint16_t height);
/** Posts a frame to the GPU for processing **/
void gpu_manager_post(uint8_t *frameBuffer);
/** Destroys the display and context safely **/
void gpu_manager_dispose(void);