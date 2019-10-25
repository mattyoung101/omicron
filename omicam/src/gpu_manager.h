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

extern GLfloat minBallData[3], maxBallData[3], minLineData[3], maxLineData[3], minBlueData[3], maxBlueData[3], minYellowData[3], maxYellowData[3];

/**
 * Creates a new OpenGL context, initialises shaders, textures, etc
 * @param width the width of the framebuffer
 * @param height the height of the framebuffer
 **/
void gpu_manager_init(uint16_t width, uint16_t height);
/** Posts a frame to the GPU for processing **/
void gpu_manager_post(MMAL_BUFFER_HEADER_T *buf);
/** Destroys the display and context safely **/
void gpu_manager_dispose(void);
/** Parses a string in the format "x,y,z" into three numbers to be stored in a uniform array **/
void gpu_manager_parse_thresh(char *threshStr, GLfloat *uniformArray);