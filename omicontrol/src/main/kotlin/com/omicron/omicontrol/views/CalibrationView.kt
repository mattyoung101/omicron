package com.omicron.omicontrol.views

import javafx.geometry.Pos
import javafx.scene.image.Image
import tornadofx.*

class CalibrationView : View() {
    init {
        reloadStylesheetsOnFocus()
        title = "Calibration View | Omicontrol"
    }

    override val root = vbox {
        setPrefSize(1600.0, 900.0)
        alignment = Pos.CENTER
    }
}
