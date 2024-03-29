/*
 * This file is part of the Omicontrol project.
 * Copyright (c) 2019-2020 Team Omicron. All rights reserved.
 *
 * Team Omicron members: Lachlan Ellis, Tynan Jones, Ethan Lo,
 * James Talkington, Matt Young.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package com.omicron.omicontrol

import RemoteDebug
import com.github.ajalt.colormath.*
import com.omicron.omicontrol.maths.ModelApproach
import com.omicron.omicontrol.views.ConnectView
import javafx.application.Platform
import javafx.geometry.Point2D
import javafx.scene.control.Alert
import javafx.scene.control.ButtonType
import javafx.scene.control.TextInputDialog
import javafx.scene.layout.Region
import javafx.scene.paint.Color
import javafx.util.Duration
import org.apache.commons.math3.fitting.WeightedObservedPoint
import org.tinylog.kotlin.Logger
import tornadofx.View
import tornadofx.ViewTransition
import kotlin.math.pow


// TODO remove this class and just put all methods in the global scope
object Utils {
    fun transitionMetro(from: View, to: View) {
        from.replaceWith(to, transition = ViewTransition.Metro(Duration.seconds(1.0)))
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
 * Convert x, a centimetre distance, to a canvas pixel distance (only considering width dimension since they're similar)
 */
fun toFieldLength(x: Double): Double {
    return x / FIELD_LENGTH_CM * FIELD_CANVAS_WIDTH
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

fun pointInCircle(point: Point2D, circlePos: Point2D, circleRadius: Double): Boolean {
    val dx = circlePos.x - point.x
    val dy = circlePos.y - point.y
    return dx * dx + dy * dy <= circleRadius * circleRadius
}

fun lerp(fromValue: Double, toValue: Double, progress: Double): Double {
    return fromValue + (toValue - fromValue) * progress
}

fun lerpColour(colour1: Color, colour2: Color, progress: Double): Color {
    return colour1.interpolate(colour2, progress)
}

fun colourSpaceToColour(colourSpace: ColourSpace, colour: IntArray): ConvertibleColor {
    return when (colourSpace) {
        is RGB.Companion -> RGB(colour[0], colour[1], colour[2])
        is HSV.Companion -> HSV(colour[0], colour[1], colour[2])
        is HSL.Companion -> HSL(colour[0], colour[1], colour[2])
        is LAB.Companion -> LAB(colour[0].toDouble(), colour[1].toDouble(), colour[2].toDouble())
        is XYZ.Companion -> XYZ(colour[0].toDouble(), colour[1].toDouble(), colour[2].toDouble())
        else -> throw IllegalArgumentException("Invalid colour space: $colourSpace with channels: $colour")
    }
}

/**
 * Calculate the coefficient of determination, or R^2 value, of the dataset
 * Source: https://en.wikipedia.org/wiki/Coefficient_of_determination#Definitions
 */
fun calculateRSquared(approach: ModelApproach, coefficients: DoubleArray, points: List<WeightedObservedPoint>): Double {
    val mean = points.sumByDouble { it.y } / points.size

    // TODO change both of these to use sumByDouble since it's more Kotlin correct
    // calculate sum of squares of residuals, SSres
    var ssRes = 0.0
    for (point in points){
        val yi = point.y
        val fi = approach.evaluate(point.x, coefficients)
        ssRes += (yi - fi).pow(2.0)
    }

    // calcualte the total sum of squares, SStot
    var ssTot = 0.0
    for (point in points){
        val yi = point.y
        ssTot += (yi - mean).pow(2.0)
    }

    // finally calculate the r squared value
    return 1 - (ssRes / ssTot)
}