#pragma once

#include <stdint.h>

/**
 * Creates a new OpenGL context and initialises the VideoCore
 * @param width the width of the framebuffer
 * @param height the height of the framebuffer
 **/
void gpu_manager_init(uint16_t width, uint16_t height);
/** Destroys the display and context safely **/
void gpu_manager_shutdown(void);