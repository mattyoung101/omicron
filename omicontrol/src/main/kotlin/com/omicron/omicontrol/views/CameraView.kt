package com.omicron.omicontrol.views

import com.google.common.eventbus.Subscribe
import com.omicron.omicontrol.Utils
import com.omicron.omicontrol.Values
import javafx.geometry.Pos
import javafx.scene.Parent
import javafx.scene.control.Alert
import javafx.scene.image.Image
import javafx.scene.image.ImageView
import tornadofx.*
import java.io.ByteArrayInputStream
import kotlin.system.exitProcess
import com.omicron.omicontrol.Utils.showWaitFixBug
import javafx.application.Platform
import javafx.scene.control.ButtonType

class CameraView : View() {
    init {
        reloadStylesheetsOnFocus()
        title = "Camera View | Omicontrol"
        Values.eventBus.register(this)
    }

    private lateinit var imageView: ImageView

    @Subscribe
    fun receiveMessageEvent(message: RemoteDebug.DebugFrame) {
//        println("Received message event with size: ${message.defaultImage.size()}")
        imageView.image = Image(ByteArrayInputStream(message.defaultImage.toByteArray()))
    }

    override val root = vbox {
        setPrefSize(1600.0, 900.0)

        menubar {
            menu("File") {
                item("Exit").setOnAction {
                    exitProcess(0)
                }
            }
            menu("Connection") {
                item("Disconnect").setOnAction {
                    Values.connectionManager.disconnect()
                    Utils.transitionMetro(this@CameraView, ConnectView())
                    tooltip.text = "Disconnect from the robot"
                }
            }
            menu("Actions") {
                item("Reboot remote")
                item("Shutdown remote")
                item("Save thresholds")
            }
            menu("Help") {
                item("About").setOnAction {
                    val alert = Alert(
                        Alert.AlertType.INFORMATION, "Copyright (c) 2019 Team Omicron. See LICENSE.txt.",
                        ButtonType.OK
                    ).apply {
                        headerText = "Omicontrol v${Values.VERSION}"
                        title = "About"
                        isResizable = true
                        setOnShown {
                            Platform.runLater {
                                isResizable = false
                                dialogPane.scene.window.sizeToScene()
                            }
                        }
                    }
                    alert.show()
                }
            }
        }


        vbox {
            hbox {
                imageView = imageview()
            }
            alignment = Pos.CENTER
        }
    }
}