package com.omicron.omicontrol

import com.omicron.omicontrol.views.ConnectView
import javafx.application.Platform
import javafx.geometry.Point2D
import javafx.scene.control.Alert
import javafx.scene.control.ButtonType
import javafx.scene.control.TextInputDialog
import javafx.scene.layout.Region
import javafx.util.Duration
import org.tinylog.kotlin.Logger
import tornadofx.View
import tornadofx.ViewTransition

// TODO remove this class and just put all methods in the global scope
object Utils {
    fun transitionMetro(from: View, to: View) {
        from.replaceWith(to, transition = ViewTransition.Metro(Duration.seconds(1.2)))
    }

    fun showGenericAlert(alertType: Alert.AlertType, contentText: String, headerText: String, titleText: String = "Omicontrol"){
        val alert = Alert(alertType, contentText, ButtonType.OK).apply {
            this.headerText = headerText
            title = titleText
            isResizable = true
            setOnShown {
                Platform.runLater {
                    isResizable = false
                    dialogPane.scene.window.sizeToScene()
                }
            }
        }
        alert.dialogPane.minHeight = Region.USE_PREF_SIZE
        alert.show()
    }

    fun showTextInputDialog(contentText: String, headerText: String, default: String = "", titleText: String = "Omicontrol"): String {
        val alert = TextInputDialog(default).apply {
            this.headerText = headerText
            title = titleText
            isResizable = true
            setContentText(contentText)
            setOnShown {
                Platform.runLater {
                    isResizable = false
                    dialogPane.scene.window.sizeToScene()
                }
            }
        }
        alert.dialogPane.minHeight = Region.USE_PREF_SIZE
        val result = alert.showAndWait()
        return if (result.isPresent) result.get() else ""
    }

    fun showConfirmDialog(contentText: String, headerText: String, titleText: String = "Omicontrol"): Boolean {
        val alert = Alert(Alert.AlertType.CONFIRMATION, contentText).apply {
            this.headerText = headerText
            title = titleText
            isResizable = true
            setOnShown {
                Platform.runLater {
                    isResizable = false
                    dialogPane.scene.window.sizeToScene()
                }
            }
        }
        alert.dialogPane.minHeight = Region.USE_PREF_SIZE

        val result = alert.showAndWait()
        return if (result.isPresent) result.get() == ButtonType.OK else false
    }

    /**
     * Disconnects the current view from Omicam
     */
    fun disconnect(currentView: View){
        Logger.debug("Disconnecting...")
        CONNECTION_MANAGER.disconnect()
        EVENT_BUS.unregister(currentView)
        transitionMetro(currentView, ConnectView())
    }

    /**
     * Converts a field measurement in cm to a screen pixels one for the canvas
     */
    fun fieldToScreen(measurement: Double): Double {
        // TODO MAKE THIS PIECE OF FUCKING SHIT WORK
        return 0.0
    }
}

/**
 * Turns a real field position in centimetres with the origin at the centre, into field canvas coordinates in pixels
 * with origin at top left corner.
 */
fun Point2D.toCanvasPosition(): Point2D {
    // this is just the inverse of the one below, we swap the variables to consider the real field instead of the canvas
    return Point2D(x / FIELD_LENGTH_CM * FIELD_CANVAS_WIDTH + (FIELD_CANVAS_WIDTH / 2.0),
        y / FIELD_WIDTH_CM * FIELD_CANVAS_HEIGHT + (FIELD_CANVAS_HEIGHT / 2.0))
}

/**
 * Turns a field canvas position in pixels with origin at top left corner, into a field position in centimetres with
 * origin at centre.
 */
fun Point2D.toRealPosition(): Point2D {
    // the process I believe for this is we scale between 0 and 1, multiply (scale to) the real field size then
    // make the origin point the centre of the field
    return Point2D(x / FIELD_CANVAS_WIDTH * FIELD_LENGTH_CM - (FIELD_LENGTH_CM / 2.0),
        y / FIELD_CANVAS_HEIGHT * FIELD_WIDTH_CM - (FIELD_WIDTH_CM / 2.0))
}

/**
 * Shortcut to send a message to Omicam asking to enable/disable sending images. In the field view, sending images
 * is disabled to save bandwidth.
 */
fun setSendFrames(sendFrames: Boolean){
    val msg = RemoteDebug.DebugCommand.newBuilder().apply {
        messageId = DebugCommands.CMD_SET_SEND_FRAMES.ordinal
        isEnabled = sendFrames
    }.build()
    CONNECTION_MANAGER.dispatchCommand(msg)
}