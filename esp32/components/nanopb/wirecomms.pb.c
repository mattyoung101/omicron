/* Automatically generated nanopb constant definitions */
/* Generated by nanopb-0.3.9.4 at Wed Jan 22 00:20:25 2020. */

#include "wirecomms.pb.h"

/* @@protoc_insertion_point(includes) */
#if PB_PROTO_HEADER_VERSION != 30
#error Regenerate this file with the current version of nanopb generator.
#endif



const pb_field_t LSlaveToMaster_fields[10] = {
    PB_FIELD(  1, FLOAT   , SINGULAR, STATIC  , FIRST, LSlaveToMaster, lineAngle, lineAngle, 0),
    PB_FIELD(  2, FLOAT   , SINGULAR, STATIC  , OTHER, LSlaveToMaster, lineSize, lineAngle, 0),
    PB_FIELD(  3, BOOL    , SINGULAR, STATIC  , OTHER, LSlaveToMaster, onLine, lineSize, 0),
    PB_FIELD(  4, BOOL    , SINGULAR, STATIC  , OTHER, LSlaveToMaster, lineOver, onLine, 0),
    PB_FIELD(  5, FLOAT   , SINGULAR, STATIC  , OTHER, LSlaveToMaster, lastAngle, lineOver, 0),
    PB_FIELD(  6, INT32   , SINGULAR, STATIC  , OTHER, LSlaveToMaster, frontLRF, lastAngle, 0),
    PB_FIELD(  7, INT32   , SINGULAR, STATIC  , OTHER, LSlaveToMaster, rightLRF, frontLRF, 0),
    PB_FIELD(  8, INT32   , SINGULAR, STATIC  , OTHER, LSlaveToMaster, backLRF, rightLRF, 0),
    PB_FIELD(  9, INT32   , SINGULAR, STATIC  , OTHER, LSlaveToMaster, leftLRF, backLRF, 0),
    PB_LAST_FIELD
};

const pb_field_t MasterToLSlave_fields[3] = {
    PB_FIELD(  1, FLOAT   , SINGULAR, STATIC  , FIRST, MasterToLSlave, heading, heading, 0),
    PB_REPEATED_FIXED_COUNT(  2, BOOL    , OTHER, MasterToLSlave, debugLEDs, heading, 0),
    PB_LAST_FIELD
};

const pb_field_t MSlaveToMaster_fields[3] = {
    PB_FIELD(  1, INT32   , SINGULAR, STATIC  , FIRST, MSlaveToMaster, mouseDX, mouseDX, 0),
    PB_FIELD(  2, INT32   , SINGULAR, STATIC  , OTHER, MSlaveToMaster, mouseDY, mouseDX, 0),
    PB_LAST_FIELD
};

const pb_field_t MasterToMSlave_fields[5] = {
    PB_FIELD(  1, INT32   , SINGULAR, STATIC  , FIRST, MasterToMSlave, frMotor, frMotor, 0),
    PB_FIELD(  2, INT32   , SINGULAR, STATIC  , OTHER, MasterToMSlave, brMotor, frMotor, 0),
    PB_FIELD(  3, INT32   , SINGULAR, STATIC  , OTHER, MasterToMSlave, blMotor, brMotor, 0),
    PB_FIELD(  4, INT32   , SINGULAR, STATIC  , OTHER, MasterToMSlave, flMotor, blMotor, 0),
    PB_LAST_FIELD
};


/* @@protoc_insertion_point(eof) */
