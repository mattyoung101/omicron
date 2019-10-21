#pragma once

// Misc constants and settings

#define VERBOSE_LOGGING 1 // whether or not verbose logging is enabled (LOG_TRACE if true, otherwise LOG_INFO)
#define ENABLE_DIAGNOSTICS 0 // whether or not diagnostics are enabled (verbose logging should be enabled too)
#define USE_RGB 1 // use RGB or use YUV colour space

// Standard port setting for the camera component
#define MMAL_CAMERA_PREVIEW_PORT 0
#define MMAL_CAMERA_VIDEO_PORT 1
#define MMAL_CAMERA_CAPTURE_PORT 2

// Video format information
// 0 implies variable
#define VIDEO_FRAME_RATE_NUM 30
#define VIDEO_FRAME_RATE_DEN 1

/// Video render needs at least 2 buffers.
#define VIDEO_OUTPUT_BUFFERS_NUM 3