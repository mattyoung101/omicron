#pragma once

// Misc constants and settings

/**
 * Version history:
 * 0.0.0a: basic CPU blob detection, remote debugger implemented
 * 1.0.0a: switch to OpenCV blob detection and have it working
 * 1.1.0a: Integration with Omicontrol completed
 * 1.2.0a: works on Jetson with optimisation and thresholding for objects
 * 1.3.0a: (WIP) cropping (ROI) optimisations, support for goal thresholding at lower resolutions
 */
#define OMICAM_VERSION "1.3.0a"

#define VERBOSE_LOGGING 1 // whether or not verbose logging is enabled (LOG_TRACE if true, otherwise LOG_INFO)

#define VISION_SCALE_FACTOR 0.3 // scale factor for goal detection frame between 0.0 and 1.0, decrease to decrease imag size
#define VISION_CROP_ENABLED 0 // whether or not to enable the ROI crop
#define VISION_DIAGNOSTICS 1 // enable or disable performance (i.e. FPS) diagnostics

#define REMOTE_JPEG_QUALITY 45 // quality of remote debugger JPEG, 0 being the worst and 100 being the best
#define REMOTE_COMPRESS_LEVEL 6 // zlib compression level for threshold masks, 0 being cheapest and 10 being most expensive
#define REMOTE_FRAME_INTERVAL 1 // send a debug frame every N real frames
#define REMOTE_PORT 42708 // which port the remote debug TCP server runs on
#define REMOTE_ENABLED 1 // whether or not remote debug is enabled
#define REMOTE_ALWAYS_SEND 0 // if true, ignore whether or not a connection exists and always send debug frames
#define REMOTE_TEMP_REPORTING_INTERVAL 2 // record the temperature every this many seconds
typedef enum {
    CMD_OK = 0, // the last command completed successfully
    CMD_POWER_OFF, // ask Omicam to shutdown the Jetson
    CMD_POWER_REBOOT, // ask Omicam to reboot the Jetson
    CMD_THRESHOLDS_GET_ALL, // return the current thresholds for all object
    CMD_THRESHOLDS_SET, // set the specified object's threshold to the given value
    CMD_THRESHOLDS_WRITE_DISK, // writes the current thresholds to the INI file and then to disk
    CMD_THRESHOLDS_SELECT, // select the particular threshold to stream
    CMD_MOVE_TO_XY, // move to the given (X,Y) coordinates on the field, will need to be forwarded to ESP
    CMD_MOVE_RESET, // move to starting position
    CMD_MOVE_HALT, // stops the robot in place, braking
    CMD_MOVE_RESUME, // allows the robot to move again
    CMD_MOVE_ORIENT, // orient to a specific direction
} debug_commands_t;
typedef enum {
    OBJ_NONE = 0,
    OBJ_BALL,
    OBJ_GOAL_YELLOW,
    OBJ_GOAL_BLUE,
    OBJ_LINES,
} field_objects_t;

#define BUILD_TARGET_SBC 0 // Omicam will be running on a SBC. All features enabled as normal.
#define BUILD_TARGET_PC 1 // Omicam will be running locally on a PC. Uses test imagery and some features are disabled.
#define BUILD_TARGET BUILD_TARGET_PC // which platform Omicam will be running on

#define LOCALISER_ERROR_TOLERANCE 2 // stop optimisation when a coordinate with this error in centimetres is found
#define LOCALISER_STEP_TOLERANCE 0.1 // stop optimisation if the last step size was smaller than this in centimetres
#define LOCALISER_MAX_EVAL_TIME 0.1 // max evaluation time for the optimiser in seconds

// error checking and other misc stuff
