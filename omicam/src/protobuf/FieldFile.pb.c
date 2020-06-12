/* Automatically generated nanopb constant definitions */
/* Generated by nanopb-0.3.9.3 at Fri Jun 12 23:42:54 2020. */

#include "FieldFile.pb.h"

/* @@protoc_insertion_point(includes) */
#if PB_PROTO_HEADER_VERSION != 30
#error Regenerate this file with the current version of nanopb generator.
#endif



const pb_field_t FieldFile_fields[6] = {
    PB_FIELD(  1, INT32   , SINGULAR, STATIC  , FIRST, FieldFile, unitDistance, unitDistance, 0),
    PB_FIELD(  2, INT32   , SINGULAR, STATIC  , OTHER, FieldFile, cellCount, unitDistance, 0),
    PB_FIELD(  3, INT32   , SINGULAR, STATIC  , OTHER, FieldFile, length, cellCount, 0),
    PB_FIELD(  4, INT32   , SINGULAR, STATIC  , OTHER, FieldFile, width, length, 0),
    PB_FIELD(  5, BYTES   , SINGULAR, STATIC  , OTHER, FieldFile, data, width, 0),
    PB_LAST_FIELD
};


/* Check that field information fits in pb_field_t */
#if !defined(PB_FIELD_32BIT)
/* If you get an error here, it means that you need to define PB_FIELD_32BIT
 * compile-time option. You can do that in pb.h or on compiler command line.
 * 
 * The reason you need to do this is that some of your messages contain tag
 * numbers or field sizes that are larger than what can fit in 8 or 16 bit
 * field descriptors.
 */
PB_STATIC_ASSERT((pb_membersize(FieldFile, data) < 65536), YOU_MUST_DEFINE_PB_FIELD_32BIT_FOR_MESSAGES_FieldFile)
#endif

#if !defined(PB_FIELD_16BIT) && !defined(PB_FIELD_32BIT)
#error Field descriptor for FieldFile.data is too large. Define PB_FIELD_16BIT to fix this.
#endif


/* @@protoc_insertion_point(eof) */
