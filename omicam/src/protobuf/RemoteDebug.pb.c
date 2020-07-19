/* Automatically generated nanopb constant definitions */
/* Generated by nanopb-0.3.9.3 at Sun Jul 19 20:45:16 2020. */

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

const pb_field_t RDPointF_fields[3] = {
    PB_FIELD(  1, FLOAT   , SINGULAR, STATIC  , FIRST, RDPointF, x, x, 0),
    PB_FIELD(  2, FLOAT   , SINGULAR, STATIC  , OTHER, RDPointF, y, x, 0),
    PB_LAST_FIELD
};

const pb_field_t RDThreshold_fields[3] = {
    PB_REPEATED_FIXED_COUNT(  2, INT32   , FIRST, RDThreshold, min, min, 0),
    PB_REPEATED_FIXED_COUNT(  3, INT32   , OTHER, RDThreshold, max, min, 0),
    PB_LAST_FIELD
};

const pb_field_t RDRobot_fields[4] = {
    PB_FIELD(  1, MESSAGE , SINGULAR, STATIC  , FIRST, RDRobot, position, position, &RDPointF_fields),
    PB_FIELD(  2, FLOAT   , SINGULAR, STATIC  , OTHER, RDRobot, orientation, position, 0),
    PB_FIELD(  3, STRING  , SINGULAR, STATIC  , OTHER, RDRobot, fsmState, orientation, 0),
    PB_LAST_FIELD
};

const pb_field_t RDObstacle_fields[4] = {
    PB_FIELD(  1, MESSAGE , SINGULAR, STATIC  , FIRST, RDObstacle, susTriBegin, susTriBegin, &RDPointF_fields),
    PB_FIELD(  2, MESSAGE , SINGULAR, STATIC  , OTHER, RDObstacle, susTriEnd, susTriBegin, &RDPointF_fields),
    PB_FIELD(  3, MESSAGE , SINGULAR, STATIC  , OTHER, RDObstacle, centroid, susTriEnd, &RDPointF_fields),
    PB_LAST_FIELD
};

const pb_field_t DebugFrame_fields[31] = {
    PB_FIELD(  1, BYTES   , SINGULAR, STATIC  , FIRST, DebugFrame, defaultImage, defaultImage, 0),
    PB_FIELD(  2, BYTES   , SINGULAR, STATIC  , OTHER, DebugFrame, ballThreshImage, defaultImage, 0),
    PB_FIELD(  3, FLOAT   , SINGULAR, STATIC  , OTHER, DebugFrame, temperature, ballThreshImage, 0),
    PB_FIELD(  4, MESSAGE , SINGULAR, STATIC  , OTHER, DebugFrame, ballRect, temperature, &RDRect_fields),
    PB_FIELD(  5, MESSAGE , SINGULAR, STATIC  , OTHER, DebugFrame, ballCentroid, ballRect, &RDPoint_fields),
    PB_FIELD(  6, INT32   , SINGULAR, STATIC  , OTHER, DebugFrame, fps, ballCentroid, 0),
    PB_FIELD(  7, INT32   , SINGULAR, STATIC  , OTHER, DebugFrame, frameWidth, fps, 0),
    PB_FIELD(  8, INT32   , SINGULAR, STATIC  , OTHER, DebugFrame, frameHeight, frameWidth, 0),
    PB_FIELD(  9, MESSAGE , SINGULAR, STATIC  , OTHER, DebugFrame, cropRect, frameHeight, &RDRect_fields),
    PB_FIELD( 10, DOUBLE  , REPEATED, STATIC  , OTHER, DebugFrame, rays, cropRect, 0),
    PB_FIELD( 11, DOUBLE  , REPEATED, STATIC  , OTHER, DebugFrame, dewarpedRays, rays, 0),
    PB_FIELD( 12, INT32   , SINGULAR, STATIC  , OTHER, DebugFrame, mirrorRadius, dewarpedRays, 0),
    PB_FIELD( 13, MESSAGE , REPEATED, STATIC  , OTHER, DebugFrame, robots, mirrorRadius, &RDRobot_fields),
    PB_FIELD( 15, FLOAT   , SINGULAR, STATIC  , OTHER, DebugFrame, rayInterval, robots, 0),
    PB_FIELD( 16, INT32   , SINGULAR, STATIC  , OTHER, DebugFrame, localiserEvals, rayInterval, 0),
    PB_FIELD( 18, STRING  , SINGULAR, STATIC  , OTHER, DebugFrame, localiserStatus, localiserEvals, 0),
    PB_FIELD( 19, MESSAGE , SINGULAR, STATIC  , OTHER, DebugFrame, ballPos, localiserStatus, &RDPointF_fields),
    PB_FIELD( 20, MESSAGE , REPEATED, STATIC  , OTHER, DebugFrame, localiserVisitedPoints, ballPos, &RDPointF_fields),
    PB_FIELD( 21, BOOL    , REPEATED, STATIC  , OTHER, DebugFrame, raysSuspicious, localiserVisitedPoints, 0),
    PB_FIELD( 22, INT32   , SINGULAR, STATIC  , OTHER, DebugFrame, localiserRate, raysSuspicious, 0),
    PB_FIELD( 23, MESSAGE , SINGULAR, STATIC  , OTHER, DebugFrame, yellowGoalPos, localiserRate, &RDPointF_fields),
    PB_FIELD( 24, MESSAGE , SINGULAR, STATIC  , OTHER, DebugFrame, blueGoalPos, yellowGoalPos, &RDPointF_fields),
    PB_FIELD( 25, BOOL    , SINGULAR, STATIC  , OTHER, DebugFrame, isYellowKnown, blueGoalPos, 0),
    PB_FIELD( 26, BOOL    , SINGULAR, STATIC  , OTHER, DebugFrame, isBallKnown, isYellowKnown, 0),
    PB_FIELD( 27, BOOL    , SINGULAR, STATIC  , OTHER, DebugFrame, isBlueKnown, isBallKnown, 0),
    PB_FIELD( 28, MESSAGE , SINGULAR, STATIC  , OTHER, DebugFrame, goalEstimate, isBlueKnown, &RDPointF_fields),
    PB_FIELD( 29, MESSAGE , SINGULAR, STATIC  , OTHER, DebugFrame, estimateMinBounds, goalEstimate, &RDPointF_fields),
    PB_FIELD( 30, MESSAGE , SINGULAR, STATIC  , OTHER, DebugFrame, estimateMaxBounds, estimateMinBounds, &RDPointF_fields),
    PB_FIELD( 31, FLOAT   , SINGULAR, STATIC  , OTHER, DebugFrame, susRayCutoff, estimateMaxBounds, 0),
    PB_FIELD( 32, MESSAGE , REPEATED, STATIC  , OTHER, DebugFrame, detectedObstacles, susRayCutoff, &RDObstacle_fields),
    PB_LAST_FIELD
};

const pb_field_t DebugCommand_fields[11] = {
    PB_FIELD(  1, INT32   , SINGULAR, STATIC  , FIRST, DebugCommand, messageId, messageId, 0),
    PB_FIELD(  2, MESSAGE , SINGULAR, STATIC  , OTHER, DebugCommand, coords, messageId, &RDPoint_fields),
    PB_FIELD(  3, FLOAT   , SINGULAR, STATIC  , OTHER, DebugCommand, orientation, coords, 0),
    PB_REPEATED_FIXED_COUNT(  4, MESSAGE , OTHER, DebugCommand, allThresholds, orientation, &RDThreshold_fields),
    PB_FIELD(  5, INT32   , SINGULAR, STATIC  , OTHER, DebugCommand, objectId, allThresholds, 0),
    PB_FIELD(  6, BOOL    , SINGULAR, STATIC  , OTHER, DebugCommand, minMax, objectId, 0),
    PB_FIELD(  7, INT32   , SINGULAR, STATIC  , OTHER, DebugCommand, colourChannel, minMax, 0),
    PB_FIELD(  8, INT32   , SINGULAR, STATIC  , OTHER, DebugCommand, value, colourChannel, 0),
    PB_FIELD(  9, INT32   , SINGULAR, STATIC  , OTHER, DebugCommand, robotId, value, 0),
    PB_FIELD( 10, BOOL    , SINGULAR, STATIC  , OTHER, DebugCommand, isEnabled, robotId, 0),
    PB_LAST_FIELD
};

const pb_field_t RDMsgFrame_fields[4] = {
    PB_FIELD(  1, MESSAGE , SINGULAR, STATIC  , FIRST, RDMsgFrame, frame, frame, &DebugFrame_fields),
    PB_FIELD(  2, MESSAGE , SINGULAR, STATIC  , OTHER, RDMsgFrame, command, frame, &DebugCommand_fields),
    PB_FIELD(  3, INT32   , SINGULAR, STATIC  , OTHER, RDMsgFrame, whichMessage, command, 0),
    PB_LAST_FIELD
};


/* Check that field information fits in pb_field_t */
#if !defined(PB_FIELD_32BIT)
#error Field descriptor for DebugFrame.ballThreshImage is too large. Define PB_FIELD_32BIT to fix this.
#endif


/* On some platforms (such as AVR), double is really float.
 * These are not directly supported by nanopb, but see example_avr_double.
 * To get rid of this error, remove any double fields from your .proto.
 */
PB_STATIC_ASSERT(sizeof(double) == 8, DOUBLE_MUST_BE_8_BYTES)

/* @@protoc_insertion_point(eof) */
