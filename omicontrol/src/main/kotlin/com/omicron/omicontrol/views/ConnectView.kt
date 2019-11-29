package com.omicron.omicontrol.views

import com.omicron.omicontrol.*
import javafx.application.Platform
import javafx.collections.FXCollections
import javafx.geometry.Pos
import javafx.scene.control.Alert
import javafx.scene.control.ButtonType
import javafx.scene.control.TextField
import javafx.scene.layout.Region
import tornadofx.*
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

            // TODO use forms for all this shit

            // title label
            hbox {
                label("Omicontrol Connection Setup") {
                    addClass(Styles.titleLabel)
                    alignment = Pos.CENTER
                }
                alignment = Pos.CENTER
            }

            hbox {
                label("Remote IP: ")
                ipField = textfield(REMOTE_IP)
                alignment = Pos.CENTER
            }

            hbox {
                label("Remote port: ")
                portField = textfield(REMOTE_PORT.toString())
                alignment = Pos.CENTER
            }

            hbox {
                label("Choose view: ")
                combobox<String> {
                    items = FXCollections.observableArrayList("Camera view", "Robot view")
                    selectionModel.selectFirst()
                }
                alignment = Pos.CENTER
            }

            hbox {
                button("Connect"){
                    setOnAction {
                        try {
                            CONNECTION_MANAGER.connect(ipField.text, portField.text.toInt())
                            Utils.transitionMetro(this@ConnectView, CameraView())
                        } catch (e: Exception){
                            Utils.showGenericAlert(Alert.AlertType.ERROR, "Error: $e\n\nPlease check the Pi is" +
                                    " powered on, and Omicam is running successfully.",
                                "Failed to establish connection to camera")
                            e.printStackTrace()
                        }
                    }
                }
                addClass(Styles.paddedBox)
                alignment = Pos.CENTER
            }

            hbox {
                button("?"){
                    setOnAction {
                        Utils.showGenericAlert(Alert.AlertType.INFORMATION,
                            """
                                Welcome to Omicontrol, the wireless debugging, controlling and monitoring app used by Team Omicron.
                                
                                To begin, please make sure you're connected to the same network as the Raspberry Pi.
                                This can be achieved to the "Omicam" hotspot, or by having the Pi connect to your Wi-Fi.
                                
                                Then, use the default IP and port to connect. If that fails, run an nmap scan or check
                                your router to find the Pi's IP address. The port will be the same (unless you changed it).
                            """.trimIndent(), "Connection Help")
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