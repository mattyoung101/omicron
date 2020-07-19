/* Automatically generated nanopb header */
/* Generated by nanopb-0.3.9.3 at Sun Jul 19 20:45:16 2020. */

#ifndef PB_UART_PB_H_INCLUDED
#define PB_UART_PB_H_INCLUDED
#include <pb.h>

/* @@protoc_insertion_point(includes) */
#if PB_PROTO_HEADER_VERSION != 30
#error Regenerate this file with the current version of nanopb generator.
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Struct definitions */
typedef struct _ESP32DebugCommand {
    int32_t msgId;
    int32_t robotId;
    int32_t orientation;
    int32_t x;
    int32_t y;
/* @@protoc_insertion_point(struct:ESP32DebugCommand) */
} ESP32DebugCommand;

typedef struct _LocalisationData {
    float estimatedX;
    float estimatedY;
/* @@protoc_insertion_point(struct:LocalisationData) */
} LocalisationData;

typedef struct _ObjectData {
    float ballAngle;
    float ballMag;
    float ballAbsX;
    float ballAbsY;
    bool ballExists;
    float goalBlueAngle;
    float goalBlueMag;
    float goalBlueAbsX;
    float goalBlueAbsY;
    bool goalBlueExists;
    float goalYellowAngle;
    float goalYellowMag;
    float goalYellowAbsX;
    float goalYellowAbsY;
    bool goalYellowExists;
/* @@protoc_insertion_point(struct:ObjectData) */
} ObjectData;

typedef struct _SensorData {
    int32_t relDspX;
    int32_t relDspY;
    int32_t absDspX;
    int32_t absDspY;
    float orientation;
    char fsmState[64];
/* @@protoc_insertion_point(struct:SensorData) */
} SensorData;

/* Default values for struct fields */

/* Initializer values for message structs */
#define ObjectData_init_default                  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
#define LocalisationData_init_default            {0, 0}
#define SensorData_init_default                  {0, 0, 0, 0, 0, ""}
#define ESP32DebugCommand_init_default           {0, 0, 0, 0, 0}
#define ObjectData_init_zero                     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
#define LocalisationData_init_zero               {0, 0}
#define SensorData_init_zero                     {0, 0, 0, 0, 0, ""}
#define ESP32DebugCommand_init_zero              {0, 0, 0, 0, 0}

/* Field tags (for use in manual encoding/decoding) */
#define ESP32DebugCommand_msgId_tag              1
#define ESP32DebugCommand_robotId_tag            2
#define ESP32DebugCommand_orientation_tag        3
#define ESP32DebugCommand_x_tag                  4
#define ESP32DebugCommand_y_tag                  5
#define LocalisationData_estimatedX_tag          1
#define LocalisationData_estimatedY_tag          2
#define ObjectData_ballAngle_tag                 1
#define ObjectData_ballMag_tag                   2
#define ObjectData_ballAbsX_tag                  3
#define ObjectData_ballAbsY_tag                  4
#define ObjectData_ballExists_tag                5
#define ObjectData_goalBlueAngle_tag             6
#define ObjectData_goalBlueMag_tag               7
#define ObjectData_goalBlueAbsX_tag              8
#define ObjectData_goalBlueAbsY_tag              9
#define ObjectData_goalBlueExists_tag            10
#define ObjectData_goalYellowAngle_tag           11
#define ObjectData_goalYellowMag_tag             12
#define ObjectData_goalYellowAbsX_tag            13
#define ObjectData_goalYellowAbsY_tag            14
#define ObjectData_goalYellowExists_tag          15
#define SensorData_relDspX_tag                   1
#define SensorData_relDspY_tag                   2
#define SensorData_absDspX_tag                   3
#define SensorData_absDspY_tag                   4
#define SensorData_orientation_tag               5
#define SensorData_fsmState_tag                  6

/* Struct field encoding specification for nanopb */
extern const pb_field_t ObjectData_fields[16];
extern const pb_field_t LocalisationData_fields[3];
extern const pb_field_t SensorData_fields[7];
extern const pb_field_t ESP32DebugCommand_fields[6];

/* Maximum encoded size of messages (where known) */
#define ObjectData_size                          66
#define LocalisationData_size                    10
#define SensorData_size                          115
#define ESP32DebugCommand_size                   55

/* Message IDs (where set with "msgid" option) */
#ifdef PB_MSGID

#define UART_MESSAGES \


#endif

#ifdef __cplusplus
} /* extern "C" */
#endif
/* @@protoc_insertion_point(eof) */

#endif
