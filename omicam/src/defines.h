#pragma once

// Misc constants and settings

#define VERBOSE_LOGGING 1 // whether or not verbose logging is enabled (LOG_TRACE if true, otherwise LOG_INFO)
#define ENABLE_DIAGNOSTICS 0 // enable or disable performance (i.e. FPS) diagnostics
#define USE_RGB 1 // use RGB or use YUV colour space
#define DEBUG_JPEG_QUALITY 30 // quality of remote debugger JPEG, 0 being the worst and 100 being the best
#define DEBUG_FRAME_EVERY 60 // send a debug frame every X real frames
#define DEBUG_USE_PNG 0 // use PNG instead of JPEG for remote debug, much slower but higher quality
#define DEBUG_WRITE_FRAME_DISK 0 // whether or not to write the frame to disk in the remote debugger
#define DEBUG_PORT 47208 // which port the remote debug TCP server runs on

// Standard port setting for the camera component
#define MMAL_CAMERA_PREVIEW_PORT 0
#define MMAL_CAMERA_VIDEO_PORT 1
#define MMAL_CAMERA_CAPTURE_PORT 2
// Video format information
#define VIDEO_FRAME_RATE_DEN 1
// Video render needs at least 2 buffers.
#define VIDEO_OUTPUT_BUFFERS_NUM 3

// error checking and other misc stuff
#if (DEBUG_WRITE_FRAME_DISK) && (DEBUG_FRAME_EVERY < 120)
#error Framerate is too fast to save files to disk. Either disable DEBUG_WRITE_FRAME_DISK or increase DEBUG_FRAME_EVERY.
#endif