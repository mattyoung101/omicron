package com.omicron.omicontrol.views

import com.omicron.omicontrol.*
import javafx.geometry.Point2D
import javafx.geometry.Pos
import javafx.scene.Parent
import javafx.scene.canvas.GraphicsContext
import javafx.scene.control.Alert
import javafx.scene.control.Label
import javafx.scene.image.Image
import javafx.scene.input.KeyCode
import javafx.scene.input.KeyCodeCombination
import javafx.scene.input.KeyCombination
import javafx.scene.layout.Priority
import javafx.scene.paint.Color
import javafx.stage.FileChooser
import net.objecthunter.exp4j.Expression
import net.objecthunter.exp4j.ExpressionBuilder
import org.greenrobot.eventbus.Subscribe
import org.tinylog.kotlin.Logger
import tornadofx.*
import java.io.ByteArrayInputStream
import java.io.FileWriter
import java.nio.file.Paths
import kotlin.system.exitProcess

/**
 * This screen displays the localised positions of the robots on a virtual field and allows you to control them
 */
class FieldView : View() {
    private lateinit var display: GraphicsContext
    private val fieldImage = Image("field_cropped_scaled.png")

    init {
        reloadStylesheetsOnFocus()
        title = "Field View | Omicontrol"
    }

    @ExperimentalUnsignedTypes
    @Subscribe
    fun receiveMessageEvent(message: RemoteDebug.DebugFrame) {
        // FIXME decode the data and check if display exists first?
        display.fill = Color.BLACK
        display.fillRect(0.0, 0.0, FIELD_CANVAS_WIDTH, FIELD_CANVAS_HEIGHT)
        display.drawImage(fieldImage, 0.0, 0.0, FIELD_CANVAS_WIDTH, FIELD_CANVAS_HEIGHT)
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
        EVENT_BUS.unregister(this@FieldView)
        Utils.transitionMetro(this@FieldView, ConnectView())
    }

    override val root = vbox {
        setPrefSize(1600.0, 1000.0)
        EVENT_BUS.register(this@FieldView)

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
            menu("Actions") {
                item("Enter sleep mode") {
                    setOnAction {
                        val msg = RemoteDebug.DebugCommand.newBuilder()
                            .setMessageId(DebugCommands.CMD_SLEEP_ENTER.ordinal)
                            .build()
                        CONNECTION_MANAGER.dispatchCommand(msg, {
                            Logger.info("Acknowledgement of sleep received, disconnecting")
                            runLater {
                                Utils.showGenericAlert(
                                    Alert.AlertType.INFORMATION,
                                    "Omicam has entered a low power sleep state.\n\nVision will not be functional until" +
                                            " you reconnect, which automatically exits sleep mode.", "Sleep mode"
                                )
                                disconnect()
                            }
                        })
                    }
                    accelerator = KeyCodeCombination(KeyCode.E, KeyCombination.CONTROL_DOWN)
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
                canvas(FIELD_CANVAS_WIDTH, FIELD_CANVAS_HEIGHT) {
                    display = graphicsContext2D
                }
                alignment = Pos.TOP_RIGHT
            }

            vbox {
                hbox {
                    label("Robot Management") {
                        addClass(Styles.bigLabel)
                        alignment = Pos.TOP_CENTER
                    }
                    hgrow = Priority.ALWAYS
                    alignment = Pos.TOP_CENTER
                }

                form {
                    fieldset {
                        field {
                            label("Selected robot: "){ addClass(Styles.boldLabel) }
                            label("None")
                        }
                    }
                    fieldset {
                        field {
                            label("Robot 0:"){ addClass(Styles.boldLabel) }
                        }
                        field {
                            label("Position: Unknown (accuracy: Unknown)")
                        }
                        field {
                            label("FSM state: Unknown")
                        }
                    }
                    fieldset {
                        field {
                            label("Robot 1:"){ addClass(Styles.boldLabel) }
                        }
                        field {
                            label("Position: Unknown (accuracy: Unknown)")
                        }
                        field {
                            label("FSM state: Unknown")
                        }
                    }

                    fieldset {
                        field {
                            label("Actions:"){ addClass(Styles.boldLabel) }
                        }

                        field {
                            button("Halt ALL robots")
                            button("Resume ALL robots")
                        }

                        field {
                            button("Halt selected")
                            button("Resume selected")
                        }

                        field {
                            button("Orient selected")
                        }

                        field {
                            button("Reset ALL")
                        }

                        field {
                            button("Switch to camera view"){

                                setOnAction {
                                    Logger.debug("Changing views")
                                    EVENT_BUS.unregister(this@FieldView)
                                    Utils.transitionMetro(this@FieldView, CameraView())
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
    }
}