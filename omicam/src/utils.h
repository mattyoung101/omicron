#pragma once
#include <bits/types/FILE.h>
#include "defines.h"
#include "protobuf/UART.pb.h"
#include <pthread.h>
#include <nlopt.h>
#include <tinyexpr.h>
#include "mathc.h"
#include <stdatomic.h>

// Globals - yes, this sucks

/** min and max thresholds for each field object */
extern int32_t minBallData[3], maxBallData[3], minLineData[3], maxLineData[3], minBlueData[3], maxBlueData[3], minYellowData[3], maxYellowData[3];
/** array of each threshold data in order */
extern int32_t *thresholds[];
extern bool isDrawRobotMask, isDrawMirrorMask;
extern char *fieldObjToString[];
/** this is the UNCROPPED video width and height (i.e. what we receive raw from the camera) */
extern int32_t videoWidth, videoHeight;
extern int32_t visionRobotMaskRadius, visionMirrorRadius;
/** (x,y,width,height) describing the frame cropping rectangle, loaded from INI file */
extern int32_t visionCropRect[4];
/** used by te_compile() when compiling the mirror model expression - TODO I still think this might be race condition prone */
extern _Atomic double mirrorModelVariable;
extern te_expr *mirrorModelExpr;
/** true if Omicam is currently in sleep mode (low power mode) */
extern bool sleeping;
extern pthread_cond_t sleepCond;
extern pthread_mutex_t sleepMutex;
/** if true, debug frames are sent to Omicontrol, otherwise they are still processed but not sent to save bandwidth */
extern bool sendDebugFrames;
/** The culmination of several months of difficult R&D: the robot's x,y position in field coordinates!!! */
extern struct vec2 localisedPosition;
extern bool visionRecordingEnabled, visionPlayback, visionDebugRobotMask;

#ifdef __cplusplus
extern "C" {
#endif

/** Parses a string in the format "x,y,z" into three numbers to be stored in the given array  */
void utils_parse_thresh(char *threshStr, int32_t *array);
/** Similar to utils_parse_thresh, but expects a 4 element rectangle of (x, y, width, height) */
void utils_parse_rect(char *rectStr, int32_t *array);
/** Gets the timestamp in milliseconds */
double utils_time_millis(void);
/** Gets the timestamp in microseconds */
double utils_time_micros(void);
/** Writes all thresholds to the INI file using their current values */
void utils_write_thresholds_disk();
/** Utility function to encode the vision data into a protobuf packet and sent it over UART, using the comms_uart module. */
void utils_cv_transmit_data(ObjectData ballData);
/** Reads a binary file from disk and its size. You must free() the returned buffer. **/
uint8_t *utils_load_bin(char *path, long *size);
/** Applies the calculated dewarp model to turn the given pixel distance into a centimetre distance in the camera. */
double utils_camera_dewarp(double x);
/**
 * Enters sleep mode (low power mode), designed to keep CPU thermals under control if no work needs to be done.
 * In this mode, networking is kept alive but vision processing (and thus also localisation) is suspended.
 * If an Omicontrol client is currently connected, it is expected to disconnect after this call.
 * When a new connection is received, Omicam will automatically wake up from sleep.
 */
void utils_sleep_enter(void);
/** Exits sleep mode (low power mode). Does nothing if already awake. */
void utils_sleep_exit(void);
/** Reloads Omicam ini config file from disk */
void utils_reload_config(void);
/** Calculates the CRC8 hash of a buffer */
uint8_t crc8(const uint8_t *data, size_t len);
/** Linearly interpolate between fromValue and toValue, by progress (between 0.0 and 1.0) */
double utils_lerp(double fromValue, double toValue, double progress);
const char *nlopt_result_to_string(nlopt_result result);
/** returns true if the given null-terminated string contains only numbers */
bool utils_only_numbers(char *s);
/** Hunts down and eliminates all other running instances of Omicam, used if THERE_CAN_BE_ONLY_ONE=1 */
uint32_t utils_go_sicko_mode(void);

#ifdef __cplusplus
}
#endif

/** first 8 bits of unsigned 16 bit int */
#define HIGH_BYTE_16(num) ((uint8_t) ((num >> 8) & 0xFF))
/** second 8 bits of unsigned 16 bit int */
#define LOW_BYTE_16(num)  ((uint8_t) ((num & 0xFF)))
/** unpack two 8 bit integers into a 16 bit integer */
#define UNPACK_16(a, b) ((uint16_t) ((a << 8) | b))
/** forces compiler to keep a variable */
#define KEEP_VAR __attribute__((used))
/** returns true if the object is a goal and would have been rescaled */
#define VISION_IS_RESCALED (objectId == OBJ_GOAL_BLUE || objectId == OBJ_GOAL_YELLOW)
/** rounds a double to an int, like Kotlin's roundToInt() function */
#define ROUND2INT(x) ((int32_t) round(x))
#define ROUND2UINT(x) ((uint32_t) round(x))
/** converts real field X coordinate into field file coordinate */
#define X_TO_FF(x) (constrain(ROUND2INT(x), 0, field.length))
/** converts real field Y coordinate into field file coordinate */
#define Y_TO_FF(y) (constrain(ROUND2INT(y), 0, field.width))
/** x squared */
#define sq(x) (x * x)

// note: put all your shit above this line because CLion fucks the indent of anything below this macro
/** sends a response indicating OK to Omicontrol */
#define RD_SEND_OK_RESPONSE do { \
    DebugCommand response = DebugCommand_init_zero; \
    response.messageId = CMD_OK; \
    send_response(response); } while (0);