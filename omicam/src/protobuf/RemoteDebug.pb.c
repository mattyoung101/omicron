/* Automatically generated nanopb constant definitions */
/* Generated by nanopb-0.3.9.3 at Tue Dec 10 16:45:18 2019. */

#include "RemoteDebug.pb.h"

/* @@protoc_insertion_point(includes) */
#if PB_PROTO_HEADER_VERSION != 30
#error Regenerate this file with the current version of nanopb generator.
#endif



const pb_field_t RDRect_fields[5] = {
    PB_FIELD(  1, INT32   , SINGULAR, STATIC  , FIRST, RDRect, x, x, 0),
    PB_FIELD(  2, INT32   , SINGULAR, STATIC  , OTHER, RDRect, y, x, 0),
    PB_FIELD(  3, INT32   , SINGULAR, STATIC  , OTHER, RDRect, width, y, 0),
    PB_FIELD(  4, INT32   , SINGULAR, STATIC  , OTHER, RDRect, height, width, 0),
    PB_LAST_FIELD
};

const pb_field_t RDPoint_fields[3] = {
    PB_FIELD(  1, INT32   , SINGULAR, STATIC  , FIRST, RDPoint, x, x, 0),
    PB_FIELD(  2, INT32   , SINGULAR, STATIC  , OTHER, RDPoint, y, x, 0),
    PB_LAST_FIELD
};

const pb_field_t DebugFrame_fields[6] = {
    PB_FIELD(  1, BYTES   , SINGULAR, STATIC  , FIRST, DebugFrame, defaultImage, defaultImage, 0),
    PB_FIELD(  2, BYTES   , SINGULAR, STATIC  , OTHER, DebugFrame, ballThreshImage, defaultImage, 0),
    PB_FIELD(  3, FLOAT   , SINGULAR, STATIC  , OTHER, DebugFrame, temperature, ballThreshImage, 0),
    PB_FIELD(  4, MESSAGE , SINGULAR, STATIC  , OTHER, DebugFrame, ballRect, temperature, &RDRect_fields),
    PB_FIELD(  5, MESSAGE , SINGULAR, STATIC  , OTHER, DebugFrame, ballCentroid, ballRect, &RDPoint_fields),
    PB_LAST_FIELD
};

const pb_field_t DebugCommand_fields[4] = {
    PB_FIELD(  1, INT32   , SINGULAR, STATIC  , FIRST, DebugCommand, messageId, messageId, 0),
    PB_FIELD(  2, INT32   , SINGULAR, STATIC  , OTHER, DebugCommand, coordX, messageId, 0),
    PB_FIELD(  3, INT32   , SINGULAR, STATIC  , OTHER, DebugCommand, coordY, coordX, 0),
    PB_LAST_FIELD
};


/* Check that field information fits in pb_field_t */
#if !defined(PB_FIELD_32BIT)
#error Field descriptor for DebugFrame.ballThreshImage is too large. Define PB_FIELD_32BIT to fix this.
#endif


/* @@protoc_insertion_point(eof) */
