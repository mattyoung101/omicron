package com.omicron.omicontrol.views

import RemoteDebug
import com.omicron.omicontrol.*
import javafx.geometry.Pos
import javafx.scene.canvas.GraphicsContext
import javafx.scene.control.Alert
import javafx.scene.image.Image
import javafx.scene.input.KeyCode
import javafx.scene.input.KeyCodeCombination
import javafx.scene.input.KeyCombination
import javafx.scene.layout.Priority
import javafx.scene.paint.Color
import org.greenrobot.eventbus.Subscribe
import org.tinylog.kotlin.Logger
import tornadofx.*
import java.io.ByteArrayInputStream
import kotlin.system.exitProcess

class DewarpPoint(val pixelDistance: Double, var realDistance: Double)

/**
 * This is essentially a stripped down version of the ConnectView which can be used to make calibrating the
 * camera dewarp model easier.
 */
class CalibrationView : View() {
    private lateinit var display: GraphicsContext
    private val points = mutableListOf<DewarpPoint>()

    init {
        reloadStylesheetsOnFocus()
        title = "Calibration View | Omicontrol"
    }

    @ExperimentalUnsignedTypes
    @Subscribe
    fun receiveMessageEvent(message: RemoteDebug.DebugFrame) {
        // decode the message, but in this case we only care about the image
        val img = Image(ByteArrayInputStream(message.defaultImage.toByteArray()))

        runLater {
            display.fill = Color.BLACK
            display.fillRect(0.0, 0.0, CANVAS_WIDTH, CANVAS_HEIGHT)
            display.drawImage(img, message.cropRect.x.toDouble(), message.cropRect.y.toDouble(), img.width, img.height)
        }
    }

    @Subscribe
    fun receiveRemoteShutdownEvent(message: RemoteShutdownEvent){
        Logger.info("Received remote shutdown event")
        runLater {
            Utils.showGenericAlert(
                Alert.AlertType.ERROR, "The remote connection has unexpectedly terminated.\n" +
                        "Please check Omicam is still running and try again.\n\nError description: Protobuf message was null",
                "Connection error")
            disconnect()
        }
    }

    private fun disconnect(){
        Logger.debug("Disconnecting...")
        CONNECTION_MANAGER.disconnect()
        EVENT_BUS.unregister(this@CalibrationView)
        Utils.transitionMetro(this@CalibrationView, ConnectView())
    }

    override val root = vbox {
        setPrefSize(1600.0, 900.0)
        EVENT_BUS.register(this@CalibrationView)

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
                        disconnect()
                    }
                    accelerator = KeyCodeCombination(KeyCode.D, KeyCombination.CONTROL_DOWN)
                }
            }
            menu("Help") {
                item("About").setOnAction {
                    Utils.showGenericAlert(
                        Alert.AlertType.INFORMATION, "Copyright (c) 2019-2020 Team Omicron. See LICENSE.txt",
                        "Omicontrol v$OMICONTROL_VERSION", "About")
                }
            }
        }

        hbox {
            vbox {
                canvas(CANVAS_WIDTH, CANVAS_HEIGHT){
                    display = graphicsContext2D
                }
                alignment = Pos.TOP_RIGHT
            }

            vbox {
                hbox {
                    label("Camera Calibration") {
                        addClass(Styles.bigLabel)
                        alignment = Pos.TOP_CENTER
                    }
                    hgrow = Priority.ALWAYS
                    alignment = Pos.TOP_CENTER
                }

                form {
                    fieldset {
                        field {
                            tableview(points.observable()){
                                readonlyColumn("Pixel distance", DewarpPoint::pixelDistance)
                                column("Real distance", DewarpPoint::realDistance)
                            }
                        }
                    }

                    fieldset {
                        field {
                            button("Clear points"){
                                setOnAction {
                                    points.clear()
                                }
                            }
                        }
                    }

                    fieldset {
                        field {
                            button("Calculate model"){
                                setOnAction {
                                    // TODO
                                }
                            }
                        }
                    }
                }

                hgrow = Priority.ALWAYS
                alignment = Pos.TOP_LEFT
            }

            hgrow = Priority.ALWAYS
        }

        vbox {
            paddingTop = 25.0
            alignment = Pos.CENTER
            button("Switch to camera view")
        }
    }
}
