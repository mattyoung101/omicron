/* Automatically generated nanopb constant definitions */
/* Generated by nanopb-0.3.9.3 at Fri Dec 13 23:36:19 2019. */

#include "UART.pb.h"

/* @@protoc_insertion_point(includes) */
#if PB_PROTO_HEADER_VERSION != 30
#error Regenerate this file with the current version of nanopb generator.
#endif



const pb_field_t BallData_fields[7] = {
    PB_FIELD(  1, FLOAT   , SINGULAR, STATIC  , FIRST, BallData, ballX, ballX, 0),
    PB_FIELD(  2, FLOAT   , SINGULAR, STATIC  , OTHER, BallData, ballY, ballX, 0),
    PB_FIELD(  3, FLOAT   , SINGULAR, STATIC  , OTHER, BallData, goalBlueX, ballY, 0),
    PB_FIELD(  4, FLOAT   , SINGULAR, STATIC  , OTHER, BallData, goalBlueY, goalBlueX, 0),
    PB_FIELD(  5, FLOAT   , SINGULAR, STATIC  , OTHER, BallData, goalYellowX, goalBlueY, 0),
    PB_FIELD(  6, FLOAT   , SINGULAR, STATIC  , OTHER, BallData, goalYellowY, goalYellowX, 0),
    PB_LAST_FIELD
};

const pb_field_t LocalisationData_fields[4] = {
    PB_FIELD(  1, FLOAT   , SINGULAR, STATIC  , FIRST, LocalisationData, estimatedX, estimatedX, 0),
    PB_FIELD(  2, FLOAT   , SINGULAR, STATIC  , OTHER, LocalisationData, estimatedY, estimatedX, 0),
    PB_FIELD(  3, FLOAT   , SINGULAR, STATIC  , OTHER, LocalisationData, error, estimatedY, 0),
    PB_LAST_FIELD
};


/* @@protoc_insertion_point(eof) */
