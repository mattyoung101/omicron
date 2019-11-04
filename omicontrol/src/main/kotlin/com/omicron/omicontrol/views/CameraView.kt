package com.omicron.omicontrol.views

import com.google.common.eventbus.Subscribe
import com.omicron.omicontrol.CONNECTION_MANAGER
import com.omicron.omicontrol.EVENT_BUS
import com.omicron.omicontrol.Utils
import com.omicron.omicontrol.VERSION
import javafx.geometry.Pos
import javafx.scene.control.Alert
import javafx.scene.image.Image
import javafx.scene.image.ImageView
import tornadofx.*
import java.io.ByteArrayInputStream
import kotlin.system.exitProcess
import javafx.application.Platform
import javafx.scene.control.ButtonType
import javafx.scene.effect.BlendMode
import javafx.scene.input.KeyCode
import javafx.scene.input.KeyCodeCombination
import javafx.scene.input.KeyCombination

class CameraView : View() {
    init {
        reloadStylesheetsOnFocus()
        title = "Camera View | Omicontrol"
        EVENT_BUS.register(this)
    }

    private lateinit var defaultImageView: ImageView
    private lateinit var threshImageView: ImageView

    @Subscribe
    fun receiveMessageEvent(message: RemoteDebug.DebugFrame) {
        defaultImageView.image = Image(ByteArrayInputStream(message.defaultImage.toByteArray()))
        threshImageView.image = Image(ByteArrayInputStream(message.threshImage.toByteArray()))
    }

    override val root = vbox {
        setPrefSize(1600.0, 900.0)

        menubar {
            menu("File") {
                item("Exit"){
                    setOnAction {
                        exitProcess(0)
                    }
                    accelerator = KeyCodeCombination(KeyCode.Q, KeyCombination.CONTROL_DOWN)
                }
            }
            menu("Connection") {
                item("Disconnect"){
                    setOnAction {
                        CONNECTION_MANAGER.disconnect()
                        Utils.transitionMetro(this@CameraView, ConnectView())
                    }
                    accelerator = KeyCodeCombination(KeyCode.D, KeyCombination.CONTROL_DOWN)
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
                        headerText = "Omicontrol v${VERSION}"
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
                // we render the image and the thresh image on top of each other. the thresh image is rendered
                // with add blending (so white pixels black pixels are see through and white pixels are white)
                // TODO in future we need to add a toggle pane to select the different channels of the image
                stackpane {
                    defaultImageView = imageview()
                    threshImageView = imageview{
                        blendMode = BlendMode.ADD
                    }
                }
                alignment = Pos.CENTER
            }
            alignment = Pos.CENTER
        }
    }
}