/* Automatically generated nanopb header */
/* Generated by nanopb-0.3.9.3 at Tue Jan 21 21:34:28 2020. */

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
typedef struct _LocalisationData {
    float estimatedX;
    float estimatedY;
    float error;
/* @@protoc_insertion_point(struct:LocalisationData) */
} LocalisationData;

typedef struct _ObjectData {
    float ballX;
    float ballY;
    bool ballExists;
    float goalBlueX;
    float goalBlueY;
    bool goalBlueExists;
    float goalYellowX;
    float goalYellowY;
    bool goalYellowExists;
/* @@protoc_insertion_point(struct:ObjectData) */
} ObjectData;

/* Default values for struct fields */

/* Initializer values for message structs */
#define ObjectData_init_default                  {0, 0, 0, 0, 0, 0, 0, 0, 0}
#define LocalisationData_init_default            {0, 0, 0}
#define ObjectData_init_zero                     {0, 0, 0, 0, 0, 0, 0, 0, 0}
#define LocalisationData_init_zero               {0, 0, 0}

/* Field tags (for use in manual encoding/decoding) */
#define LocalisationData_estimatedX_tag          1
#define LocalisationData_estimatedY_tag          2
#define LocalisationData_error_tag               3
#define ObjectData_ballX_tag                     1
#define ObjectData_ballY_tag                     2
#define ObjectData_ballExists_tag                3
#define ObjectData_goalBlueX_tag                 4
#define ObjectData_goalBlueY_tag                 5
#define ObjectData_goalBlueExists_tag            6
#define ObjectData_goalYellowX_tag               7
#define ObjectData_goalYellowY_tag               8
#define ObjectData_goalYellowExists_tag          9

/* Struct field encoding specification for nanopb */
extern const pb_field_t ObjectData_fields[10];
extern const pb_field_t LocalisationData_fields[4];

/* Maximum encoded size of messages (where known) */
#define ObjectData_size                          36
#define LocalisationData_size                    15

/* Message IDs (where set with "msgid" option) */
#ifdef PB_MSGID

#define UART_MESSAGES \


#endif

#ifdef __cplusplus
} /* extern "C" */
#endif
/* @@protoc_insertion_point(eof) */

#endif
