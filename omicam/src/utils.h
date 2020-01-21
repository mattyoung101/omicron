#pragma once
#include <GLES2/gl2.h>
#include <bits/types/FILE.h>
#include "defines.h"
#include "protobuf/UART.pb.h"

// Globals
extern int32_t minBallData[3], maxBallData[3], minLineData[3], maxLineData[3], minBlueData[3], maxBlueData[3], minYellowData[3], maxYellowData[3];
extern int32_t *thresholds[];
extern char *fieldObjToString[];
/** this is the UNCROPPED video width and height (i.e. what we receive raw from the camera) */
extern int32_t videoWidth, videoHeight, visionRobotMaskRadius, visionMirrorRadius;
extern int32_t visionCropRect[4];

#ifdef __cplusplus
extern "C" {
#endif

/** Parses a string in the format "x,y,z" into three numbers to be stored in the given array  **/
void utils_parse_thresh(char *threshStr, int32_t *array);
/** Similar to utils_parse_thresh, but expects a 4 element rectangle of (x, y, width, height) **/
void utils_parse_rect(char *rectStr, int32_t *array);
/** Gets the timestamp in milliseconds **/
double utils_time_millis();
/** Writes all thresholds to the INI file using their current values **/
void utils_write_thresholds_disk();
/**
 * Utility function to encode the vision data into a protobuf packet and sent it over UART, using the
 * comms_uart module.
 */
void utils_cv_transmit_data(ObjectData ballData);
/** Reads a binary file from disk. You must free() the returned buffer. **/
uint8_t *utils_load_bin(char *path, long *size);

#ifdef __cplusplus
}
#endif

/** first 8 bits of unsigned 16 bit int **/
#define HIGH_BYTE_16(num) ((uint8_t) ((num >> 8) & 0xFF))
/** second 8 bits of unsigned 16 bit int **/
#define LOW_BYTE_16(num)  ((uint8_t) ((num & 0xFF)))
/** unpack two 8 bit integers into a 16 bit integer **/
#define UNPACK_16(a, b) ((uint16_t) ((a << 8) | b))
#define KEEP_VAR __attribute__((used))
#define RD_SEND_OK_RESPONSE do { DebugCommand response = DebugCommand_init_zero; \
    response.messageId = CMD_OK; \
    send_response(response); } while (0)
#define VISION_IS_RESCALED (objectId == OBJ_GOAL_BLUE || objectId == OBJ_GOAL_YELLOW)
#define ROUND2INT(x) ((int32_t) round(x))