/* Automatically generated nanopb header */
/* Generated by nanopb-0.3.9.3 at Sun Jul 19 20:45:16 2020. */

#ifndef PB_REMOTEDEBUG_PB_H_INCLUDED
#define PB_REMOTEDEBUG_PB_H_INCLUDED
#include <pb.h>

/* @@protoc_insertion_point(includes) */
#if PB_PROTO_HEADER_VERSION != 30
#error Regenerate this file with the current version of nanopb generator.
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Struct definitions */
typedef struct _RDPoint {
    int32_t x;
    int32_t y;
/* @@protoc_insertion_point(struct:RDPoint) */
} RDPoint;

typedef struct _RDPointF {
    float x;
    float y;
/* @@protoc_insertion_point(struct:RDPointF) */
} RDPointF;

typedef struct _RDRect {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
/* @@protoc_insertion_point(struct:RDRect) */
} RDRect;

typedef struct _RDThreshold {
    int32_t min[3];
    int32_t max[3];
/* @@protoc_insertion_point(struct:RDThreshold) */
} RDThreshold;

typedef struct _DebugCommand {
    int32_t messageId;
    RDPoint coords;
    float orientation;
    RDThreshold allThresholds[5];
    int32_t objectId;
    bool minMax;
    int32_t colourChannel;
    int32_t value;
    int32_t robotId;
    bool isEnabled;
/* @@protoc_insertion_point(struct:DebugCommand) */
} DebugCommand;

typedef struct _RDObstacle {
    RDPointF susTriBegin;
    RDPointF susTriEnd;
    RDPointF centroid;
/* @@protoc_insertion_point(struct:RDObstacle) */
} RDObstacle;

typedef struct _RDRobot {
    RDPointF position;
    float orientation;
    char fsmState[64];
/* @@protoc_insertion_point(struct:RDRobot) */
} RDRobot;

typedef PB_BYTES_ARRAY_T(128000) DebugFrame_defaultImage_t;
typedef PB_BYTES_ARRAY_T(128000) DebugFrame_ballThreshImage_t;
typedef struct _DebugFrame {
    DebugFrame_defaultImage_t defaultImage;
    DebugFrame_ballThreshImage_t ballThreshImage;
    float temperature;
    RDRect ballRect;
    RDPoint ballCentroid;
    int32_t fps;
    int32_t frameWidth;
    int32_t frameHeight;
    RDRect cropRect;
    pb_size_t rays_count;
    double rays[128];
    pb_size_t dewarpedRays_count;
    double dewarpedRays[128];
    int32_t mirrorRadius;
    pb_size_t robots_count;
    RDRobot robots[2];
    float rayInterval;
    int32_t localiserEvals;
    char localiserStatus[33];
    RDPointF ballPos;
    pb_size_t localiserVisitedPoints_count;
    RDPointF localiserVisitedPoints[128];
    pb_size_t raysSuspicious_count;
    bool raysSuspicious[128];
    int32_t localiserRate;
    RDPointF yellowGoalPos;
    RDPointF blueGoalPos;
    bool isYellowKnown;
    bool isBallKnown;
    bool isBlueKnown;
    RDPointF goalEstimate;
    RDPointF estimateMinBounds;
    RDPointF estimateMaxBounds;
    float susRayCutoff;
    pb_size_t detectedObstacles_count;
    RDObstacle detectedObstacles[4];
/* @@protoc_insertion_point(struct:DebugFrame) */
} DebugFrame;

typedef struct _RDMsgFrame {
    DebugFrame frame;
    DebugCommand command;
    int32_t whichMessage;
/* @@protoc_insertion_point(struct:RDMsgFrame) */
} RDMsgFrame;

/* Default values for struct fields */

/* Initializer values for message structs */
#define RDRect_init_default                      {0, 0, 0, 0}
#define RDPoint_init_default                     {0, 0}
#define RDPointF_init_default                    {0, 0}
#define RDThreshold_init_default                 {{0, 0, 0}, {0, 0, 0}}
#define RDRobot_init_default                     {RDPointF_init_default, 0, ""}
#define RDObstacle_init_default                  {RDPointF_init_default, RDPointF_init_default, RDPointF_init_default}
#define DebugFrame_init_default                  {{0, {0}}, {0, {0}}, 0, RDRect_init_default, RDPoint_init_default, 0, 0, 0, RDRect_init_default, 0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 0, 0, {RDRobot_init_default, RDRobot_init_default}, 0, 0, "", RDPointF_init_default, 0, {RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default}, 0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 0, RDPointF_init_default, RDPointF_init_default, 0, 0, 0, RDPointF_init_default, RDPointF_init_default, RDPointF_init_default, 0, 0, {RDObstacle_init_default, RDObstacle_init_default, RDObstacle_init_default, RDObstacle_init_default}}
#define DebugCommand_init_default                {0, RDPoint_init_default, 0, {RDThreshold_init_default, RDThreshold_init_default, RDThreshold_init_default, RDThreshold_init_default, RDThreshold_init_default}, 0, 0, 0, 0, 0, 0}
#define RDMsgFrame_init_default                  {DebugFrame_init_default, DebugCommand_init_default, 0}
#define RDRect_init_zero                         {0, 0, 0, 0}
#define RDPoint_init_zero                        {0, 0}
#define RDPointF_init_zero                       {0, 0}
#define RDThreshold_init_zero                    {{0, 0, 0}, {0, 0, 0}}
#define RDRobot_init_zero                        {RDPointF_init_zero, 0, ""}
#define RDObstacle_init_zero                     {RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero}
#define DebugFrame_init_zero                     {{0, {0}}, {0, {0}}, 0, RDRect_init_zero, RDPoint_init_zero, 0, 0, 0, RDRect_init_zero, 0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 0, 0, {RDRobot_init_zero, RDRobot_init_zero}, 0, 0, "", RDPointF_init_zero, 0, {RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero}, 0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 0, RDPointF_init_zero, RDPointF_init_zero, 0, 0, 0, RDPointF_init_zero, RDPointF_init_zero, RDPointF_init_zero, 0, 0, {RDObstacle_init_zero, RDObstacle_init_zero, RDObstacle_init_zero, RDObstacle_init_zero}}
#define DebugCommand_init_zero                   {0, RDPoint_init_zero, 0, {RDThreshold_init_zero, RDThreshold_init_zero, RDThreshold_init_zero, RDThreshold_init_zero, RDThreshold_init_zero}, 0, 0, 0, 0, 0, 0}
#define RDMsgFrame_init_zero                     {DebugFrame_init_zero, DebugCommand_init_zero, 0}

/* Field tags (for use in manual encoding/decoding) */
#define RDPoint_x_tag                            1
#define RDPoint_y_tag                            2
#define RDPointF_x_tag                           1
#define RDPointF_y_tag                           2
#define RDRect_x_tag                             1
#define RDRect_y_tag                             2
#define RDRect_width_tag                         3
#define RDRect_height_tag                        4
#define RDThreshold_min_tag                      2
#define RDThreshold_max_tag                      3
#define DebugCommand_messageId_tag               1
#define DebugCommand_coords_tag                  2
#define DebugCommand_orientation_tag             3
#define DebugCommand_allThresholds_tag           4
#define DebugCommand_objectId_tag                5
#define DebugCommand_minMax_tag                  6
#define DebugCommand_colourChannel_tag           7
#define DebugCommand_value_tag                   8
#define DebugCommand_robotId_tag                 9
#define DebugCommand_isEnabled_tag               10
#define RDObstacle_susTriBegin_tag               1
#define RDObstacle_susTriEnd_tag                 2
#define RDObstacle_centroid_tag                  3
#define RDRobot_position_tag                     1
#define RDRobot_orientation_tag                  2
#define RDRobot_fsmState_tag                     3
#define DebugFrame_defaultImage_tag              1
#define DebugFrame_ballThreshImage_tag           2
#define DebugFrame_temperature_tag               3
#define DebugFrame_ballRect_tag                  4
#define DebugFrame_ballCentroid_tag              5
#define DebugFrame_fps_tag                       6
#define DebugFrame_frameWidth_tag                7
#define DebugFrame_frameHeight_tag               8
#define DebugFrame_cropRect_tag                  9
#define DebugFrame_rays_tag                      10
#define DebugFrame_dewarpedRays_tag              11
#define DebugFrame_mirrorRadius_tag              12
#define DebugFrame_robots_tag                    13
#define DebugFrame_rayInterval_tag               15
#define DebugFrame_localiserEvals_tag            16
#define DebugFrame_localiserStatus_tag           18
#define DebugFrame_ballPos_tag                   19
#define DebugFrame_localiserVisitedPoints_tag    20
#define DebugFrame_raysSuspicious_tag            21
#define DebugFrame_localiserRate_tag             22
#define DebugFrame_yellowGoalPos_tag             23
#define DebugFrame_blueGoalPos_tag               24
#define DebugFrame_isYellowKnown_tag             25
#define DebugFrame_isBallKnown_tag               26
#define DebugFrame_isBlueKnown_tag               27
#define DebugFrame_goalEstimate_tag              28
#define DebugFrame_estimateMinBounds_tag         29
#define DebugFrame_estimateMaxBounds_tag         30
#define DebugFrame_susRayCutoff_tag              31
#define DebugFrame_detectedObstacles_tag         32
#define RDMsgFrame_frame_tag                     1
#define RDMsgFrame_command_tag                   2
#define RDMsgFrame_whichMessage_tag              3

/* Struct field encoding specification for nanopb */
extern const pb_field_t RDRect_fields[5];
extern const pb_field_t RDPoint_fields[3];
extern const pb_field_t RDPointF_fields[3];
extern const pb_field_t RDThreshold_fields[3];
extern const pb_field_t RDRobot_fields[4];
extern const pb_field_t RDObstacle_fields[4];
extern const pb_field_t DebugFrame_fields[31];
extern const pb_field_t DebugCommand_fields[11];
extern const pb_field_t RDMsgFrame_fields[4];

/* Maximum encoded size of messages (where known) */
#define RDRect_size                              44
#define RDPoint_size                             22
#define RDPointF_size                            10
#define RDThreshold_size                         66
#define RDRobot_size                             83
#define RDObstacle_size                          36
#define DebugFrame_size                          261009
#define DebugCommand_size                        428
#define RDMsgFrame_size                          261455

/* Message IDs (where set with "msgid" option) */
#ifdef PB_MSGID

#define REMOTEDEBUG_MESSAGES \


#endif

#ifdef __cplusplus
} /* extern "C" */
#endif
/* @@protoc_insertion_point(eof) */

#endif
