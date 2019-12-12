package com.omicron.omicontrol

import javafx.application.Platform
import javafx.scene.control.Alert
import javafx.scene.control.ButtonType
import javafx.scene.control.TextInputDialog
import javafx.scene.layout.Region
import javafx.util.Duration
import tornadofx.View
import tornadofx.ViewTransition
import java.util.*

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
}