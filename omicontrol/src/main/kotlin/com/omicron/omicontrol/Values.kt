package com.omicron.omicontrol

import javafx.scene.control.Label
import org.apache.commons.lang3.SystemUtils
import org.greenrobot.eventbus.EventBus
import java.util.concurrent.atomic.AtomicInteger
import java.util.concurrent.atomic.AtomicLong

/**
 * Version history:
 * 0.0a: basic application and communication implemented
 * 0.1a: works with new OpenCV stuff
 * 1.1a: UI implemented and threshold sliders/debug commands work properly
 * 1.2a: works with Omicam cropping and downscaling
 * 2.2a: added support for camera calibration
 * 2.3a: added sleep mode
 * 3.3a: (WIP) added field display screen
 */
const val OMICONTROL_VERSION = "3.3a"
const val REMOTE_IP = "127.0.0.1"
const val REMOTE_PORT = 42708

const val FIELD_LENGTH_CM = 243 // long side, the top and bottom
const val FIELD_WIDTH_CM = 182 // short side, the sides of the field
val CANVAS_WIDTH = if (SystemUtils.IS_OS_MAC) 1280.0 * 0.90 else 1280.0
val CANVAS_HEIGHT = if (SystemUtils.IS_OS_MAC) 720.0 * 0.90 else 720.0
// for the canvas we scale it down to 90% of the size and if it's Mac we also scale it down again
val FIELD_CANVAS_WIDTH = if (SystemUtils.IS_OS_MAC) 1280.0 * 0.9 * 0.90 else 1280.0 * 0.9
val FIELD_CANVAS_HEIGHT = if (SystemUtils.IS_OS_MAC) 962.0 * 0.9 * 0.90 else 962.0 * 0.9
val ROBOT_CANVAS_DIAMETER = 21.0 / FIELD_LENGTH_CM * FIELD_CANVAS_WIDTH // diameter of our robot on field canvas
val BALL_CANVAS_DIAMETER = 6.5 / FIELD_LENGTH_CM * FIELD_CANVAS_WIDTH // diameter of ball on field canvas

val CONNECTION_MANAGER = ConnectionManager()
val EVENT_BUS = EventBus.getDefault()!!
const val DEBUG_CAMERA_VIEW = false
const val GRAB_SEND_TIMER_PERIOD = 100L // ms
/** bytes sent/received in the last second **/
var BANDWIDTH = 0L
val COLOURS = listOf("R", "G", "B")
var lastPingLabel: Label? = null

enum class DebugCommands {
    CMD_OK, // the last command completed successfully
    CMD_ERROR, // the last command had a problem with it
    CMD_SLEEP_ENTER, // enter sleep mode (low power mode)
    CMD_THRESHOLDS_GET_ALL, // return the current thresholds for all object
    CMD_THRESHOLDS_SET, // set the specified object's threshold to the given value
    CMD_THRESHOLDS_WRITE_DISK, // writes the current thresholds to the INI file and then to disk
    CMD_THRESHOLDS_SELECT, // select the particular threshold to stream
    CMD_MOVE_TO_XY, // move to the given (X,Y) coordinates on the field, will need to be forwarded to ESP
    CMD_MOVE_RESET, // move to starting position
    CMD_MOVE_HALT, // stops the robot in place, braking
    CMD_MOVE_RESUME, // allows the robot to move again
    CMD_MOVE_ORIENT, // orient to a specific direction
    CMD_SET_SEND_FRAMES, // set whether or not to send frames (useful for saving data in the field view)
}

/** objects on the field **/
enum class FieldObjects {
    OBJ_NONE,
    OBJ_BALL,
    OBJ_GOAL_YELLOW,
    OBJ_GOAL_BLUE,
    OBJ_LINES
};