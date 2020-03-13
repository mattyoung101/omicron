package com.omicron.omicontrol.field

import javafx.geometry.Point2D

/** Generic field object with position and isKnown **/
data class FieldObject(var position: Point2D = Point2D(0.0, 0.0), var isPositionKnown: Boolean = false) {
}