/* Automatically generated nanopb constant definitions */
/* Generated by nanopb-0.3.9.3 at Sun Jan  5 14:43:49 2020. */

#include "FieldFile.pb.h"

/* @@protoc_insertion_point(includes) */
#if PB_PROTO_HEADER_VERSION != 30
#error Regenerate this file with the current version of nanopb generator.
#endif



const pb_field_t FieldFile_fields[6] = {
    PB_FIELD(  1, INT32   , SINGULAR, STATIC  , FIRST, FieldFile, unitDistance, unitDistance, 0),
    PB_FIELD(  2, INT32   , SINGULAR, STATIC  , OTHER, FieldFile, cellCount, unitDistance, 0),
    PB_FIELD(  3, INT32   , SINGULAR, STATIC  , OTHER, FieldFile, fieldWidth, cellCount, 0),
    PB_FIELD(  4, INT32   , SINGULAR, STATIC  , OTHER, FieldFile, fieldHeight, fieldWidth, 0),
    PB_FIELD(  5, FLOAT   , REPEATED, CALLBACK, OTHER, FieldFile, data, fieldHeight, 0),
    PB_LAST_FIELD
};


/* @@protoc_insertion_point(eof) */
