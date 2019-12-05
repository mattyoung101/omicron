#pragma once

// Misc constants and settings
// TODO I think at lot of these should be editable with ini

#define VERBOSE_LOGGING 1 // whether or not verbose logging is enabled (LOG_TRACE if true, otherwise LOG_INFO)
#define ENABLE_DIAGNOSTICS 0 // enable or disable performance (i.e. FPS) diagnostics
#define USE_RGB 1 // use RGB or use YUV colour space

#define DEBUG_JPEG_QUALITY 30 // quality of remote debugger JPEG, 0 being the worst and 100 being the best
#define DEBUG_COMPRESSION_LEVEL 6 // zlib compression level for remote debugger, 0 being cheapest and 10 being most expensive
#define DEBUG_FRAME_EVERY 5 // send a debug frame every X real frames
#define DEBUG_PORT 42708 // which port the remote debug TCP server runs on
#define DEBUG_ENABLED 1 // whether or not remote debug is enabled
#define DEBUG_ALWAYS_SEND 0 // if true, ignore whether or not a connection exists and always send debug frames
#define DEBUG_TEMP_REPORTING_INTERVAL 2 // record the every this many seconds

#define BLOB_USE_NEON 0 // TODO (deprecate and remove this) whether or not to enable NEON optimisations in blob detection
#define BLOB_NUM_THREADS 4 // number of worker threads for blob detector

#define BUILD_TARGET_JETSON 0 // Omicam will be running on an NVIDIA Jetson Nano
#define BUILD_TARGET_PC 1 // Omicam will be running locally on a PC (assumes no camera available and uses test imagery)
#define BUILD_TARGET BUILD_TARGET_PC

#define LOCALISE_NUM_THREADS 4 // number of worker threads used by localisation raycaster

// Standard port setting for the camera component
#define MMAL_CAMERA_PREVIEW_PORT 0
#define MMAL_CAMERA_VIDEO_PORT 1
#define MMAL_CAMERA_CAPTURE_PORT 2
// Video format information
#define VIDEO_FRAME_RATE_DEN 1
// Video render needs at least 2 buffers.
#define VIDEO_OUTPUT_BUFFERS_NUM 3

// error checking and other misc stuff
