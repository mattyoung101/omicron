/* Automatically generated nanopb constant definitions */
/* Generated by nanopb-0.3.9.3 at Wed Feb 19 21:01:25 2020. */

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


/* @@protoc_insertion_point(eof) */
