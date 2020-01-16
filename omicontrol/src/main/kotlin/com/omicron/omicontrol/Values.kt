package com.omicron.omicontrol

import com.google.common.eventbus.EventBus
import javafx.scene.control.Label
import org.apache.commons.lang3.SystemUtils

/**
 * Version history:
 * 0.0.0a: basic application and communication implemented
 * 0.1.0a: works with new OpenCV stuff
 * 1.1.0a: UI implemented and threshold sliders/debug commands work properly
 */
const val OMICONTROL_VERSION = "1.1.0a"
const val REMOTE_IP = "127.0.0.1"
const val REMOTE_PORT = 42708
val CONNECTION_MANAGER = ConnectionManager()
val EVENT_BUS = EventBus()
val CANVAS_WIDTH = if (SystemUtils.IS_OS_MAC) 1280.0 * 0.90 else 1280.0
val CANVAS_HEIGHT = if (SystemUtils.IS_OS_MAC) 720.0 * 0.90 else 720.0
const val DEBUG_CAMERA_VIEW = false
val COLOURS = listOf("R", "G", "B")
var lastPingLabel: Label? = null
const val GRAB_SEND_TIMER_PERIOD = 100L // ms

enum class DebugCommands {
    CMD_OK, // the last command completed successfully
    CMD_POWER_OFF, // ask Omicam to shutdown the SBC
    CMD_POWER_REBOOT, // ask Omicam to reboot the SBC
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