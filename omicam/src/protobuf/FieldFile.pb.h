/* Automatically generated nanopb header */
/* Generated by nanopb-0.3.9.3 at Mon Jul 13 23:03:26 2020. */

#ifndef PB_FIELDFILE_PB_H_INCLUDED
#define PB_FIELDFILE_PB_H_INCLUDED
#include <pb.h>

/* @@protoc_insertion_point(includes) */
#if PB_PROTO_HEADER_VERSION != 30
#error Regenerate this file with the current version of nanopb generator.
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Struct definitions */
typedef PB_BYTES_ARRAY_T(45000) FieldFile_data_t;
typedef struct _FieldFile {
    int32_t unitDistance;
    int32_t cellCount;
    int32_t length;
    int32_t width;
    FieldFile_data_t data;
/* @@protoc_insertion_point(struct:FieldFile) */
} FieldFile;

/* Default values for struct fields */

/* Initializer values for message structs */
#define FieldFile_init_default                   {0, 0, 0, 0, {0, {0}}}
#define FieldFile_init_zero                      {0, 0, 0, 0, {0, {0}}}

/* Field tags (for use in manual encoding/decoding) */
#define FieldFile_unitDistance_tag               1
#define FieldFile_cellCount_tag                  2
#define FieldFile_length_tag                     3
#define FieldFile_width_tag                      4
#define FieldFile_data_tag                       5

/* Struct field encoding specification for nanopb */
extern const pb_field_t FieldFile_fields[6];

/* Maximum encoded size of messages (where known) */
#define FieldFile_size                           45048

/* Message IDs (where set with "msgid" option) */
#ifdef PB_MSGID

#define FIELDFILE_MESSAGES \


#endif

#ifdef __cplusplus
} /* extern "C" */
#endif
/* @@protoc_insertion_point(eof) */

#endif
