/* Automatically generated nanopb header */
/* Generated by nanopb-0.3.9.3 at Tue Dec 10 16:45:18 2019. */

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
typedef struct _BallData {
    float ballX;
    float ballY;
    float goalBlueX;
    float goalBlueY;
    float goalYellowX;
    float goalYellowY;
/* @@protoc_insertion_point(struct:BallData) */
} BallData;

typedef struct _LocalisationData {
    float estimatedX;
    float estimatedY;
    float error;
/* @@protoc_insertion_point(struct:LocalisationData) */
} LocalisationData;

/* Default values for struct fields */

/* Initializer values for message structs */
#define BallData_init_default                    {0, 0, 0, 0, 0, 0}
#define LocalisationData_init_default            {0, 0, 0}
#define BallData_init_zero                       {0, 0, 0, 0, 0, 0}
#define LocalisationData_init_zero               {0, 0, 0}

/* Field tags (for use in manual encoding/decoding) */
#define BallData_ballX_tag                       1
#define BallData_ballY_tag                       2
#define BallData_goalBlueX_tag                   3
#define BallData_goalBlueY_tag                   4
#define BallData_goalYellowX_tag                 5
#define BallData_goalYellowY_tag                 6
#define LocalisationData_estimatedX_tag          1
#define LocalisationData_estimatedY_tag          2
#define LocalisationData_error_tag               3

/* Struct field encoding specification for nanopb */
extern const pb_field_t BallData_fields[7];
extern const pb_field_t LocalisationData_fields[4];

/* Maximum encoded size of messages (where known) */
#define BallData_size                            30
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