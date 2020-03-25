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
 * 4.4b: (WIP) UART comms implemented, improved localiser with quadrants, fixed some bugs/oversights
 */
#define OMICAM_VERSION "4.4b"
/** whether or not verbose logging is enabled (LOG_TRACE if true, otherwise LOG_INFO) */
#define VERBOSE_LOGGING 1

/** scale factor for goal detection frame between 0.0 and 1.0, decrease to decrease image size */
#define VISION_SCALE_FACTOR 0.3
/** whether or not to enable the ROI crop */
#define VISION_CROP_ENABLED 1
/** enable or disable performance (i.e. FPS) diagnostics */
#define VISION_DIAGNOSTICS 1
/** enables CLAHE adaptive histogram normalisation to correct dogshit venue lighting, bad for performance */
#define VISION_APPLY_CLAHE 0
/** path to test data used in BUILD_TARGET_PC */
#define VISION_TEST_FILE "../test_data/field5.png"

/** send a debug frame every N real frames */
#define REMOTE_FRAME_INTERVAL 1
/** quality of remote debugger JPEG, 0 being the worst and 100 being the best */
#define REMOTE_JPEG_QUALITY 75
/** zlib compression level for threshold masks, 0 being cheapest and 10 being most expensive */
#define REMOTE_COMPRESS_LEVEL 6
/** which port the remote debug TCP server runs on */
#define REMOTE_PORT 42708
/** whether or not remote debug is enabled */
#define REMOTE_ENABLED 1
/** if true, ignore whether or not a connection exists and always send debug frames */
#define REMOTE_ALWAYS_SEND 0
/** record the temperature every this many seconds */
#define REMOTE_TEMP_REPORTING_INTERVAL 2

/** Omicam will be running on a SBC. All features enabled as normal. */
#define BUILD_TARGET_SBC 0
/** Omicam will be running locally on a PC. Uses test imagery and some features are disabled. */
#define BUILD_TARGET_PC 1
/** which platform Omicam will be running on */
#define BUILD_TARGET (BUILD_TARGET_PC)

/** stop optimisation when a coordinate with this error in centimetres is found */
#define LOCALISER_ERROR_TOLERANCE 1
/** stop optimisation if the last step size was smaller than this in centimetres */
#define LOCALISER_STEP_TOLERANCE 0.1
/** max evaluation time for the optimiser in milliseconds */
#define LOCALISER_MAX_EVAL_TIME 100
/**
 * Step size to use if a goal estimate is available (original is about 60), decided with trial & error.
 * If this value is too low/high, accuracy will suffer.
 */
#define LOCALISER_SMALL_STEP 10.0
/**
 * If a goal estimate is available, NLopt bounds are constrained to a square of this size (in cm) around the estimated position.
 * Set this value conservatively, because if it's too small and the goal magnitudes suck, you run into the real risk
 * of missing the correct position. Finally, please note that this value is extremely sensitive, small changes can
 * increase/decrease the average evaluations by almost 10.
 */
#define LOCALISER_ESTIMATE_BOUNDS 70
/** The number of rays to use when raycasting on line images, generally 64 or 128 are fine. */
#define LOCALISER_NUM_RAYS 64
/** error which is 200-300 points higher than the highest usual error in the localiser, determine through trial & error */
#define LOCALISER_LARGE_ERROR 8600
/** if true, renders the objective function bitmap, raycast test bitmap and then quits */
#define LOCALISER_DEBUG 0
/** if true, prints localisation performance information to the console (just like vision diagnostics) */
#define LOCALISER_DIAGNOSTICS 1

/** if true, UART will be forced to be enabled even in BUILD_TARGET_PC */
#define UART_OVERRIDE 0
/** the Linux file for the UART device Omicam writes to, usually /dev/ttyACM0 */
#define UART_BUS_NAME "/dev/ttyACM0"

// maths defines are the same ones used in the ESP32 project

#define PI 3.14159265358979323846
#define PI2 6.283185307179586
/** multiply to convert degrees to radians */
#define DEG_RAD 0.017453292519943295
/** multiply to convert radians to degrees */
#define RAD_DEG 57.29577951308232

// put all enums and other non-defines here

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
/** enum for messages dispatched to the ESP32 firmware */
typedef enum {
    /** generic message */
    MSG_ANY = 0,
    /** object data containing positions for field objects to ESP32 */
    OBJECT_DATA,
    /** determined (x,y) positions to ESP32 **/
    LOCALISATION_DATA,
    /** mouse sensor and sensor data to Omicam **/
    SENSOR_DATA,
    /** debug command to ESP32 **/
    DEBUG_CMD,
} comms_msg_type_t;
