package com.omicron.omicontrol.views

import com.omicron.omicontrol.*
import javafx.collections.FXCollections
import javafx.geometry.Pos
import javafx.scene.control.Alert
import javafx.scene.control.ComboBox
import javafx.scene.control.TextField
import javafx.scene.image.Image
import javafx.scene.layout.Priority
import javafx.scene.paint.Color
import org.tinylog.kotlin.Logger
import tornadofx.*
import java.util.prefs.Preferences
import kotlin.system.exitProcess

class ConnectView : View() {
    private lateinit var viewBox: ComboBox<String>
    init {
        reloadStylesheetsOnFocus()
        title = "Connect to Robot | Omicontrol"
    }
    private val prefs = Preferences.userRoot().node("Omicontrol")

    override val root = vbox {
        setPrefSize(1600.0, 900.0)
        primaryStage.icons.add(Image("omicontrol_icon3.png"))
        alignment = Pos.CENTER

        stackpane {
            hbox {
                imageview("omicontrol_logo.png")
                alignment = Pos.BOTTOM_RIGHT
                paddingBottom = 8.0
                paddingRight = 8.0
            }

            hbox {
                label("v${OMICONTROL_VERSION} | Copyright (c) 2019-2020 Team Omicron."){
                    textFill = Color.LIGHTGREY
                }
                alignment = Pos.BOTTOM_LEFT
                paddingLeft = 6.0
                paddingBottom = 6.0
            }

            vbox {
                lateinit var ipField: TextField
                lateinit var portField: TextField

                // TODO use forms for all this shit

                // title label
                hbox {
                    label("Omicontrol Connection Setup") {
                        addClass(Styles.titleLabel)
                        alignment = Pos.CENTER
                        textFill = Color.WHITE
                    }
                    alignment = Pos.CENTER
                }

                hbox {
                    label("Remote IP: "){ textFill = Color.WHITE }
                    val lastIp = prefs["LastIP", DEFAULT_IP]
                    ipField = textfield(lastIp)
                    alignment = Pos.CENTER
                }

                hbox {
                    label("Remote port: "){ textFill = Color.WHITE }
                    portField = textfield(DEFAULT_PORT.toString())
                    alignment = Pos.CENTER
                }

                hbox {
                    label("Choose view: "){ textFill = Color.WHITE }
                    viewBox = combobox {
                        items = FXCollections.observableArrayList(
                            "Camera view",
                            "Field view",
                            "Calibration view")
                        val last = prefs["LastViewSelected", "Camera view"]
                        selectionModel.select(last)
                    }
                    alignment = Pos.CENTER
                }

                @Suppress("ConstantConditionIf")
                if (DEBUG_CAMERA_VIEW) {
                    button("Force camera view") {
                        setOnAction {
                            Logger.debug("Forcing transition to camera view")
                            Utils.transitionMetro(this@ConnectView, CameraView())
                        }
                    }
                    addClass(Styles.paddedBox)
                }

                hbox {
                    button("Connect") {
                        setOnAction {
                            try {
                                CONNECTION_MANAGER.connect(ipField.text, portField.text.toInt())
                                prefs.put("LastViewSelected", viewBox.value)
                                prefs.put("LastIP", ipField.text)

                                when (viewBox.value){
                                    "Camera view" -> Utils.transitionMetro(this@ConnectView, CameraView())
                                    "Calibration view" -> Utils.transitionMetro(this@ConnectView, CalibrationView())
                                    "Field view" -> Utils.transitionMetro(this@ConnectView, FieldView())
                                    else -> Logger.error("Unsupported view: ${viewBox.value}")
                                }

                            } catch (e: Exception) {
                                Utils.showGenericAlert(
                                    Alert.AlertType.ERROR, "Error: $e\n\nPlease check the device is" +
                                            " powered on, and Omicam is running successfully.",
                                    "Failed to establish connection"
                                )
                                e.printStackTrace()
                            }
                        }
                    }
                    addClass(Styles.paddedBox)
                    alignment = Pos.CENTER
                }

                hbox {
                    button("Quit") {
                        setOnAction {
                            exitProcess(0)
                        }
                    }
                    addClass(Styles.paddedBox)
                    alignment = Pos.CENTER
                }
                hgrow = Priority.ALWAYS
                vgrow = Priority.ALWAYS
                alignment = Pos.CENTER
            }

            hgrow = Priority.ALWAYS
            vgrow = Priority.ALWAYS
        }
    }
}