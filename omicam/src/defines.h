#pragma once

// Misc constants and settings

/*
 * Version history:
 * 0.0.0a: basic CPU blob detection, remote debugger implemented
 * 1.0.0a: switch to OpenCV blob detection and have it working
 * 1.1.0a: (WORK IN PROGRESS) Omicontrol integration works*
 *
 */
#define OMICAM_VERSION "1.0.0a"

#define VERBOSE_LOGGING 1 // whether or not verbose logging is enabled (LOG_TRACE if true, otherwise LOG_INFO)
#define ENABLE_DIAGNOSTICS 0 // enable or disable performance (i.e. FPS) diagnostics

#define VISION_SCALE_FACTOR 0.4 // scale factor for goal detection frame between 0.0 and 1.0

#define DEBUG_JPEG_QUALITY 30 // quality of remote debugger JPEG, 0 being the worst and 100 being the best
#define DEBUG_COMPRESSION_LEVEL 6 // zlib compression level for threshold masks, 0 being cheapest and 10 being most expensive
#define DEBUG_FRAME_EVERY 5 // send a debug frame every N real frames
#define DEBUG_PORT 42708 // which port the remote debug TCP server runs on
#define DEBUG_ENABLED 1 // whether or not remote debug is enabled
#define DEBUG_ALWAYS_SEND 0 // if true, ignore whether or not a connection exists and always send debug frames
#define DEBUG_TEMP_REPORTING_INTERVAL 2 // record the temperature every this many seconds
enum debug_commands_t {
    CMD_OK = 0, // the last command completed successfully
    CMD_POWER, // ask Omicam to shutdown the Jetson
    CMD_POWER_REBOOT, // ask Omicam to reboot the Jetson
    CMD_THRESHOLDS_GET_ALL, // return the current thresholds for all object
    CMD_THRESHOLDS_SET, // set the specified object's threshold to the given value
    CMD_THRESHOLDS_WRITE_DISK, // writes the current thresholds to the INI file and then to disk
    CMD_MOVE_TO_XY, // move to the given (X,Y) coordinates on the field, will need to be forwarded to ESP
    CMD_MOVE_RESET, // move to starting position
    CMD_MOVE_HALT, // stops the robot in place, braking
    CMD_MOVE_RESUME, // allows the robot to move again
    CMD_MOVE_ORIENT, // orient to a specific direction
};
enum field_objects_t {
    OBJ_NONE = 0,
    OBJ_BALL,
    OBJ_GOAL_YELLOW,
    OBJ_GOAL_BLUE,
    OBJ_LINES,
};

#define BUILD_TARGET_JETSON 0 // Omicam will be running on an NVIDIA Jetson Nano
#define BUILD_TARGET_PC 1 // Omicam will be running locally on a PC (assumes no camera available and uses test imagery)
#define BUILD_TARGET BUILD_TARGET_PC // which platform Omicam will be running on

#define LOCALISE_NUM_THREADS 4 // number of worker threads used by localisation raycaster (probably will be OpenCL)

// error checking and other misc stuff
