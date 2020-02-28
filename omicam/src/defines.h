#pragma once
// Misc constants and settings

/**
 * Version history:
 * 0.0a: basic CPU blob detection, remote debugger implemented
 * 1.0a: switch to OpenCV blob detection and have it working
 * 1.1a: Integration with Omicontrol completed
 * 1.2a: works on Jetson with optimisation and thresholding for objects
 * 1.3a: cropping (ROI) optimisations, support for goal thresholding at lower resolutions
 * 1.4a: performance optimisations for new SBC, new FPS timing code, fixes
 * 2.4a: localisation prototype implemented, sleep mode added
 * 3.4b: Omicam enters beta, localiser work continued
 * 3.5b: (WIP) UART comms implemented... what else?
 */
#define OMICAM_VERSION "3.5b"
#define VERBOSE_LOGGING 1 // whether or not verbose logging is enabled (LOG_TRACE if true, otherwise LOG_INFO)
#define CRANK_THE_MFIN_HOG 0 // if enabled, force high-performance CPU frequency governing and disable thermal throttling on Omicam startup

#define VISION_SCALE_FACTOR 0.3 // scale factor for goal detection frame between 0.0 and 1.0, decrease to decrease image size
#define VISION_CROP_ENABLED 1 // whether or not to enable the ROI crop
#define VISION_DIAGNOSTICS 1 // enable or disable performance (i.e. FPS) diagnostics
#define VISION_APPLY_CLAHE 0 // enables CLAHE adaptive histogram normalisation to correct dogshit venue lighting, bad for performance

#define REMOTE_FRAME_INTERVAL 1 // send a debug frame every N real frames
#define REMOTE_JPEG_QUALITY 75 // quality of remote debugger JPEG, 0 being the worst and 100 being the best
#define REMOTE_COMPRESS_LEVEL 6 // zlib compression level for threshold masks, 0 being cheapest and 10 being most expensive
#define REMOTE_PORT 42708 // which port the remote debug TCP server runs on
#define REMOTE_ENABLED 1 // whether or not remote debug is enabled
#define REMOTE_ALWAYS_SEND 0 // if true, ignore whether or not a connection exists and always send debug frames
#define REMOTE_TEMP_REPORTING_INTERVAL 2 // record the temperature every this many seconds
typedef enum {
    CMD_OK = 0, // the last command completed successfully
    CMD_ERROR, // the last command had a problem with it
    CMD_SLEEP_ENTER, // enter sleep mode (low power mode)
    CMD_THRESHOLDS_GET_ALL, // return the current thresholds for all object
    CMD_THRESHOLDS_SET, // set the specified object's threshold to the given value
    CMD_THRESHOLDS_WRITE_DISK, // writes the current thresholds to the INI file and then to disk
    CMD_THRESHOLDS_SELECT, // select the particular threshold to stream
    CMD_MOVE_TO_XY, // move to the given (X,Y) coordinates on the field, will need to be forwarded to ESP
    CMD_MOVE_RESET, // move to starting position
    CMD_MOVE_HALT, // stops the robot in place, braking
    CMD_MOVE_RESUME, // allows the robot to move again
    CMD_MOVE_ORIENT, // orient to a specific direction
    CMD_SET_SEND_FRAMES, // set whether or not to send frames (useful for saving data in the field view)
    CMD_RELOAD_CONFIG, // reloads Omicam INI config from disk
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
#define BUILD_TARGET (BUILD_TARGET_PC) // which platform Omicam will be running on

#define LOCALISER_ERROR_TOLERANCE 1 // stop optimisation when a coordinate with this error in centimetres is found
#define LOCALISER_STEP_TOLERANCE 0.01 // stop optimisation if the last step size was smaller than this in centimetres
#define LOCALISER_MAX_EVAL_TIME 100 // max evaluation time for the optimiser in milliseconds
#define LOCALISER_NUM_RAYS 128 // the number of rays to use when raycasting on the line image
#define LOCALISER_DEBUG 0 // if true, renders the objective function bitmap and quits
#define LOCALISER_DIAGNOSTICS 1 // if true, print data like vision diagnostics to console

#define PI 3.14159265359
#define PI2 6.28318530718