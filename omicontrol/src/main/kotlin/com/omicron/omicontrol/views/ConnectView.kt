package com.omicron.omicontrol.views

import com.google.common.eventbus.Subscribe
import com.omicron.omicontrol.Styles
import com.omicron.omicontrol.Utils
import com.omicron.omicontrol.Values
import javafx.geometry.Pos
import javafx.scene.control.TextField
import tornadofx.*
import java.net.ConnectException

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
                            // TODO display popup
                            e.printStackTrace()
                        }
                    }
                }

                addClass(Styles.paddedBox)
                alignment = Pos.CENTER
            }

            alignment = Pos.CENTER
        }
    }
}