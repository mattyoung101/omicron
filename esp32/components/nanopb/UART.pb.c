/* Automatically generated nanopb constant definitions */
/* Generated by nanopb-0.3.9.3 at Tue Mar 10 09:37:20 2020. */

#include "UART.pb.h"

/* @@protoc_insertion_point(includes) */
#if PB_PROTO_HEADER_VERSION != 30
#error Regenerate this file with the current version of nanopb generator.
#endif



const pb_field_t ObjectData_fields[10] = {
    PB_FIELD(  1, FLOAT   , SINGULAR, STATIC  , FIRST, ObjectData, ballAngle, ballAngle, 0),
    PB_FIELD(  2, FLOAT   , SINGULAR, STATIC  , OTHER, ObjectData, ballMag, ballAngle, 0),
    PB_FIELD(  3, BOOL    , SINGULAR, STATIC  , OTHER, ObjectData, ballExists, ballMag, 0),
    PB_FIELD(  4, FLOAT   , SINGULAR, STATIC  , OTHER, ObjectData, goalBlueAngle, ballExists, 0),
    PB_FIELD(  5, FLOAT   , SINGULAR, STATIC  , OTHER, ObjectData, goalBlueMag, goalBlueAngle, 0),
    PB_FIELD(  6, BOOL    , SINGULAR, STATIC  , OTHER, ObjectData, goalBlueExists, goalBlueMag, 0),
    PB_FIELD(  7, FLOAT   , SINGULAR, STATIC  , OTHER, ObjectData, goalYellowAngle, goalBlueExists, 0),
    PB_FIELD(  8, FLOAT   , SINGULAR, STATIC  , OTHER, ObjectData, goalYellowMag, goalYellowAngle, 0),
    PB_FIELD(  9, BOOL    , SINGULAR, STATIC  , OTHER, ObjectData, goalYellowExists, goalYellowMag, 0),
    PB_LAST_FIELD
};

const pb_field_t LocalisationData_fields[3] = {
    PB_FIELD(  1, FLOAT   , SINGULAR, STATIC  , FIRST, LocalisationData, estimatedX, estimatedX, 0),
    PB_FIELD(  2, FLOAT   , SINGULAR, STATIC  , OTHER, LocalisationData, estimatedY, estimatedX, 0),
    PB_LAST_FIELD
};

const pb_field_t MouseData_fields[5] = {
    PB_FIELD(  1, INT32   , SINGULAR, STATIC  , FIRST, MouseData, relDspX, relDspX, 0),
    PB_FIELD(  2, INT32   , SINGULAR, STATIC  , OTHER, MouseData, relDspY, relDspX, 0),
    PB_FIELD(  3, INT32   , SINGULAR, STATIC  , OTHER, MouseData, absDspX, relDspY, 0),
    PB_FIELD(  4, INT32   , SINGULAR, STATIC  , OTHER, MouseData, absDspY, absDspX, 0),
    PB_LAST_FIELD
};

const pb_field_t ESP32DebugCommand_fields[6] = {
    PB_FIELD(  1, INT32   , SINGULAR, STATIC  , FIRST, ESP32DebugCommand, msgId, msgId, 0),
    PB_FIELD(  2, INT32   , SINGULAR, STATIC  , OTHER, ESP32DebugCommand, robotId, msgId, 0),
    PB_FIELD(  3, INT32   , SINGULAR, STATIC  , OTHER, ESP32DebugCommand, orientation, robotId, 0),
    PB_FIELD(  4, INT32   , SINGULAR, STATIC  , OTHER, ESP32DebugCommand, x, orientation, 0),
    PB_FIELD(  5, INT32   , SINGULAR, STATIC  , OTHER, ESP32DebugCommand, y, x, 0),
    PB_LAST_FIELD
};


/* @@protoc_insertion_point(eof) */
