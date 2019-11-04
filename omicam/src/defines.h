#pragma once

// Misc constants and settings

#define VERBOSE_LOGGING 1 // whether or not verbose logging is enabled (LOG_TRACE if true, otherwise LOG_INFO)
#define ENABLE_DIAGNOSTICS 0 // enable or disable performance (i.e. FPS) diagnostics
#define USE_RGB 1 // use RGB or use YUV colour space
#define DEBUG_JPEG_QUALITY 30 // quality of remote debugger JPEG, 0 being the worst and 100 being the best
#define DEBUG_FRAME_EVERY 5 // send a debug frame every X real frames
#define DEBUG_PORT 42708 // which port the remote debug TCP server runs on

// Standard port setting for the camera component
#define MMAL_CAMERA_PREVIEW_PORT 0
#define MMAL_CAMERA_VIDEO_PORT 1
#define MMAL_CAMERA_CAPTURE_PORT 2
// Video format information
#define VIDEO_FRAME_RATE_DEN 1
// Video render needs at least 2 buffers.
#define VIDEO_OUTPUT_BUFFERS_NUM 3

// error checking and other misc stuff
