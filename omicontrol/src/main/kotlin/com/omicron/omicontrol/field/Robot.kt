package com.omicron.omicontrol.field

import javafx.geometry.Point2D
import javafx.scene.control.Label

data class Robot(var id: Int,
                 var position: Point2D = Point2D(0.0, 0.0),
                 var isPositionKnown: Boolean = false,
                 var fsmState: String = "Unknown",
                 var positionLabel: Label? = null) {
}