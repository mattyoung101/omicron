package com.omicron.omicontrol

import javafx.application.Platform
import javafx.scene.control.Alert
import javafx.scene.control.ButtonType
import javafx.scene.layout.Region
import javafx.util.Duration
import tornadofx.View
import tornadofx.ViewTransition

object Utils {
    fun transitionMetro(from: View, to: View) {
        from.replaceWith(to, transition = ViewTransition.Metro(Duration.seconds(1.5)))
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
}