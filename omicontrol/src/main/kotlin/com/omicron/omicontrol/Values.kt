package com.omicron.omicontrol

import com.google.common.eventbus.EventBus
import javafx.scene.control.Label

/**
 * Version history:
 * 0.0.0a: basic application and communication implemented
 * 0.1.0a: works with new OpenCV stuff
 * 1.1.0a: (WORK IN PROGRESS) UI implemented and threshold sliders/debug commands work properly
 */
const val OMICONTROL_VERSION = "1.1.0a"
const val REMOTE_IP = "127.0.0.1"
const val REMOTE_PORT = 42708
val CONNECTION_MANAGER = ConnectionManager()
val EVENT_BUS = EventBus()
const val IMAGE_WIDTH = 1280.0
const val IMAGE_HEIGHT = 720.0
const val DEBUG_CAMERA_VIEW = false
val COLOURS = listOf("R", "G", "B")
/** last ping (round trip time) in ms, only updated when sending a command **/
var LAST_PING = 0L
var lastPingLabel: Label? = null

enum class DebugCommands {
    CMD_OK, // the last command completed successfully
    CMD_POWER_OFF, // ask Omicam to shutdown the Jetson
    CMD_POWER_REBOOT, // ask Omicam to reboot the Jetson
    CMD_THRESHOLDS_GET_ALL, // return the current thresholds for all object
    CMD_THRESHOLDS_SET, // set the specified object's threshold to the given value
    CMD_THRESHOLDS_WRITE_DISK, // writes the current thresholds to the INI file and then to disk
    CMD_THRESHOLDS_SELECT, // select the particular threshold to stream
    CMD_MOVE_TO_XY, // move to the given (X,Y) coordinates on the field, will need to be forwarded to ESP
    CMD_MOVE_RESET, // move to starting position
    CMD_MOVE_HALT, // stops the robot in place, braking
    CMD_MOVE_RESUME, // allows the robot to move again
    CMD_MOVE_ORIENT, // orient to a specific direction
}

/** objects on the field **/
enum class FieldObjects {
    OBJ_NONE,
    OBJ_BALL,
    OBJ_GOAL_YELLOW,
    OBJ_GOAL_BLUE,
    OBJ_LINES
};