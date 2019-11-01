package com.omicron.omicontrol.views

import com.google.common.eventbus.Subscribe
import com.omicron.omicontrol.Styles
import com.omicron.omicontrol.Utils
import com.omicron.omicontrol.Values
import javafx.application.Platform
import javafx.geometry.Pos
import javafx.scene.control.Alert
import javafx.scene.control.ButtonType
import javafx.scene.control.TextField
import javafx.scene.layout.Region
import tornadofx.*
import java.net.ConnectException
import kotlin.system.exitProcess

class ConnectView : View() {
    init {
        reloadStylesheetsOnFocus()
        title = "Connect to Robot | Omicontrol"
    }

    override val root = vbox {
        setPrefSize(1600.0, 900.0)
        alignment = Pos.CENTER

        vbox {
            lateinit var ipField: TextField
            lateinit var portField: TextField

            // title label
            hbox {
                label("Connect to Robot") {
                    addClass(Styles.titleLabel)
                    alignment = Pos.CENTER
                }
                alignment = Pos.CENTER
            }

            hbox {
                label("Remote IP: ")
                ipField = textfield(Values.REMOTE_IP)
                alignment = Pos.CENTER
            }

            hbox {
                label("Remote port: ")
                portField = textfield(Values.REMOTE_PORT.toString())
                alignment = Pos.CENTER
            }

            hbox {
                button("Connect"){
                    setOnAction {
                        try {
                            Values.connectionManager.connect(ipField.text, portField.text.toInt())
                            Utils.transitionMetro(this@ConnectView, CameraView())
                        } catch (e: Exception){
                            val alert = Alert(
                                Alert.AlertType.ERROR, e.toString(), ButtonType.OK
                            ).apply {
                                headerText = "Failed to connect to remote"
                                title = "Connection error"
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
                            e.printStackTrace()
                        }
                    }
                }
                addClass(Styles.paddedBox)
                alignment = Pos.CENTER
            }

            hbox {
                button("Quit"){
                    setOnAction {
                        exitProcess(0)
                    }
                }
                addClass(Styles.paddedBox)
                alignment = Pos.CENTER
            }

            alignment = Pos.CENTER
        }
    }
}