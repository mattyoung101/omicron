package com.omicron.omicontrol

import javafx.application.Platform
import javafx.scene.control.Alert
import javafx.scene.control.ButtonType
import javafx.util.Duration
import tornadofx.View
import tornadofx.ViewTransition

object Utils {
    fun transitionMetro(from: View, to: View) {
        from.replaceWith(to, transition = ViewTransition.Metro(Duration.seconds(1.5)))
    }

    /**
     * Shows the alert while fixing the sizing bug that exists on KDE on Linux
     */
    fun Alert.showWaitFixBug() {
        isResizable = true
        setOnShown {
            Platform.runLater {
                isResizable = false
                dialogPane.scene.window.sizeToScene()
            }
        }
        showAndWait()
    }
}