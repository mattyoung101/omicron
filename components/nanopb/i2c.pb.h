/* Automatically generated nanopb header */
/* Generated by nanopb-0.3.9.3 at Sat Aug 03 13:57:15 2019. */

#ifndef PB_I2C_PB_H_INCLUDED
#define PB_I2C_PB_H_INCLUDED
#include <pb.h>

/* @@protoc_insertion_point(includes) */
#if PB_PROTO_HEADER_VERSION != 30
#error Regenerate this file with the current version of nanopb generator.
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Struct definitions */
typedef struct _I2CMasterProvide {
    int32_t motorFL;
    int32_t motorFR;
    int32_t motorBL;
    int32_t motorBR;
    float heading;
/* @@protoc_insertion_point(struct:I2CMasterProvide) */
} I2CMasterProvide;

typedef struct _I2CSlaveProvide {
    float lineAngle;
    float lineSize;
    bool onLine;
    bool lineOver;
    float lastAngle;
    float heading;
/* @@protoc_insertion_point(struct:I2CSlaveProvide) */
} I2CSlaveProvide;

typedef struct _SensorUpdate {
    float tsopAngle;
    float tsopStrength;
    float lineAngle;
    float lineSize;
    bool onLine;
    bool lineOver;
    float lastAngle;
    float heading;
    int32_t temperature;
    float voltage;
/* @@protoc_insertion_point(struct:SensorUpdate) */
} SensorUpdate;

/* Default values for struct fields */

/* Initializer values for message structs */
#define SensorUpdate_init_default                {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
#define I2CSlaveProvide_init_default             {0, 0, 0, 0, 0, 0}
#define I2CMasterProvide_init_default            {0, 0, 0, 0, 0}
#define SensorUpdate_init_zero                   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
#define I2CSlaveProvide_init_zero                {0, 0, 0, 0, 0, 0}
#define I2CMasterProvide_init_zero               {0, 0, 0, 0, 0}

/* Field tags (for use in manual encoding/decoding) */
#define I2CMasterProvide_motorFL_tag             1
#define I2CMasterProvide_motorFR_tag             2
#define I2CMasterProvide_motorBL_tag             3
#define I2CMasterProvide_motorBR_tag             4
#define I2CMasterProvide_heading_tag             5
#define I2CSlaveProvide_lineAngle_tag            1
#define I2CSlaveProvide_lineSize_tag             2
#define I2CSlaveProvide_onLine_tag               3
#define I2CSlaveProvide_lineOver_tag             4
#define I2CSlaveProvide_lastAngle_tag            5
#define I2CSlaveProvide_heading_tag              6
#define SensorUpdate_tsopAngle_tag               1
#define SensorUpdate_tsopStrength_tag            2
#define SensorUpdate_lineAngle_tag               3
#define SensorUpdate_lineSize_tag                4
#define SensorUpdate_onLine_tag                  5
#define SensorUpdate_lineOver_tag                6
#define SensorUpdate_lastAngle_tag               7
#define SensorUpdate_heading_tag                 8
#define SensorUpdate_temperature_tag             9
#define SensorUpdate_voltage_tag                 10

/* Struct field encoding specification for nanopb */
extern const pb_field_t SensorUpdate_fields[11];
extern const pb_field_t I2CSlaveProvide_fields[7];
extern const pb_field_t I2CMasterProvide_fields[6];

/* Maximum encoded size of messages (where known) */
#define SensorUpdate_size                        50
#define I2CSlaveProvide_size                     24
#define I2CMasterProvide_size                    49

/* Message IDs (where set with "msgid" option) */
#ifdef PB_MSGID

#define I2C_MESSAGES \


#endif

#ifdef __cplusplus
} /* extern "C" */
#endif
/* @@protoc_insertion_point(eof) */

#endif