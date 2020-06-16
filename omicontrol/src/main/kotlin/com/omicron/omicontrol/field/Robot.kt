package com.omicron.omicontrol.field

import javafx.geometry.Point2D
import javafx.scene.control.Label

// TODO make this inherit from FieldObject
data class Robot(var id: Int,
                 var position: Point2D = Point2D(0.0, 0.0),
                 var orientation: Float = 0.0f,
                 var isPositionKnown: Boolean = false,
                 var fsmState: String = "Unknown",
                 var positionLabel: Label? = null) {
}