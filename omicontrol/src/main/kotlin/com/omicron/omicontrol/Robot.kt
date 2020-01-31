package com.omicron.omicontrol

import javafx.geometry.Point2D

data class Robot(var position: Point2D = Point2D(0.0, 0.0), var positionKnown: Boolean = false,
                 var fsmState: String = "UNKNOWN") {
}