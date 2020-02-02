package com.omicron.omicontrol.views

import com.omicron.omicontrol.*
import javafx.geometry.Point2D
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
import kotlin.system.exitProcess
import com.omicron.omicontrol.field.Ball
import com.omicron.omicontrol.field.Robot
import javafx.scene.control.Label
import org.apache.commons.io.FileUtils

/**
 * This screen displays the localised positions of the robots on a virtual field and allows you to control them
 */
class FieldView : View() {
    private lateinit var display: GraphicsContext
    private lateinit var bandwidthLabel: Label
    private val fieldImage = Image("field_cropped_scaled.png")
    private val targetIcon = Image("target.png")
    private val robotImage = Image("robot.png")
    private val robots = listOf(
        Robot(),
        Robot()
    )
    private var selectedRobot: Robot? = null
    private var targetPos: Point2D? = null
    private val ball = Ball()
    private var bandwidthRecordingBegin = System.currentTimeMillis()

    init {
        reloadStylesheetsOnFocus()
        title = "Field View | Omicontrol"
    }

    @ExperimentalUnsignedTypes
    @Subscribe
    fun receiveMessageEvent(message: RemoteDebug.DebugFrame) {
        // TODO set robot positions and ball position

        runLater {
            display.fill = Color.BLACK
            display.fillRect(0.0, 0.0, FIELD_CANVAS_WIDTH, FIELD_CANVAS_HEIGHT)
            display.drawImage(fieldImage, 0.0, 0.0, FIELD_CANVAS_WIDTH, FIELD_CANVAS_HEIGHT)

            // render robots
            for (robot in robots) {
                if (robot.isPositionKnown) {
                    val half = ROBOT_CANVAS_DIAMETER / 2.0
                    val pos = robot.position.toCanvasPosition()
                    display.drawImage(robotImage, pos.x - half, pos.y - half, ROBOT_CANVAS_DIAMETER, ROBOT_CANVAS_DIAMETER)
                }
            }

            // render ball
            if (ball.isPositionKnown) {
                display.fill = Color.ORANGE
                val pos = ball.position.toCanvasPosition()
                display.fillOval(pos.x, pos.y, BALL_CANVAS_DIAMETER, BALL_CANVAS_DIAMETER)
            }

            // render target
            if (targetPos != null) {
                display.drawImage(targetIcon, targetPos!!.x - 16, targetPos!!.y - 16, 48.0, 48.0)
            }

            // update labels
            if (System.currentTimeMillis() - bandwidthRecordingBegin >= 1000) {
                bandwidthLabel.text = "Bandwidth: ${FileUtils.byteCountToDisplaySize(BANDWIDTH)}/s"
                bandwidthRecordingBegin = System.currentTimeMillis()
                BANDWIDTH = 0
            }
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
            Utils.disconnect(this@FieldView)
        }
    }

    /** checks that a robot is currently selected **/
    private fun checkSelected(): Boolean {
        return if (selectedRobot == null){
            Utils.showGenericAlert(Alert.AlertType.ERROR, "Please select a robot first.", "No robot selected")
            false
        } else {
            true
        }
    }

    override val root = vbox {
        setPrefSize(1600.0, 1000.0)
        EVENT_BUS.register(this@FieldView)
        setSendFrames(false)

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
                        Utils.disconnect(this@FieldView)
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
                                Utils.disconnect(this@FieldView)
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

                    setOnMouseClicked {
                        val fieldCanvasPos = Point2D(it.x, it.y)
                        val fieldRealPos = fieldCanvasPos.toRealPosition()
                        Logger.info("Mouse clicked at real: $fieldRealPos, canvas: $fieldCanvasPos, back to canvas: ${fieldRealPos.toCanvasPosition()}")

                        // behaviour for this:
                        // if clicking on the currently selected robot, deselect it
                        // if robot is currently selected and clicked on field, set a target to that position and move there
                        targetPos = Point2D(it.x, it.y)
                    }
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
                            label("Selected robot:"){ addClass(Styles.boldLabel) }
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
                            bandwidthLabel = label("Packet size: None recorded")
                        }
                    }

                    fieldset {
                        field {
                            label("Actions:"){ addClass(Styles.boldLabel) }
                        }

                        field {
                            button("Halt ALL robots"){
                                setOnAction {
                                    val cmd = RemoteDebug.DebugCommand.newBuilder().apply {
                                        messageId = DebugCommands.CMD_MOVE_HALT.ordinal
                                        robotId = -1 // target all
                                    }.build()
                                    CONNECTION_MANAGER.dispatchCommand(cmd)
                                }
                            }
                            button("Resume ALL robots"){
                                setOnAction {
                                    val cmd = RemoteDebug.DebugCommand.newBuilder().apply {
                                        messageId = DebugCommands.CMD_MOVE_RESUME.ordinal
                                        robotId = -1 // target all
                                    }.build()
                                    CONNECTION_MANAGER.dispatchCommand(cmd)
                                }
                            }
                        }

                        field {
                            button("Halt selected"){
                                setOnAction {
                                    if (checkSelected()){
                                        val cmd = RemoteDebug.DebugCommand.newBuilder().apply {
                                            messageId = DebugCommands.CMD_MOVE_HALT.ordinal
                                            robotId = robots.indexOf(selectedRobot)
                                        }.build()
                                        CONNECTION_MANAGER.dispatchCommand(cmd)
                                    }
                                }
                            }
                            button("Resume selected"){
                                setOnAction {
                                    if (checkSelected()){
                                        val cmd = RemoteDebug.DebugCommand.newBuilder().apply {
                                            messageId = DebugCommands.CMD_MOVE_RESUME.ordinal
                                            robotId = robots.indexOf(selectedRobot)
                                        }.build()
                                        CONNECTION_MANAGER.dispatchCommand(cmd)
                                    }
                                }
                            }
                        }

                        field {
                            button("Orient selected"){
                                setOnAction {
                                    if (checkSelected()){
                                        // FIXME ask for orientation here
                                        val cmd = RemoteDebug.DebugCommand.newBuilder().apply {
                                            messageId = DebugCommands.CMD_MOVE_ORIENT.ordinal
                                            robotId = robots.indexOf(selectedRobot)
                                            orientation = 0.0f
                                        }.build()
                                        CONNECTION_MANAGER.dispatchCommand(cmd)
                                    }
                                }
                            }
                        }

                        field {
                            button("Reset ALL"){
                                setOnAction {
                                    val cmd = RemoteDebug.DebugCommand.newBuilder().apply {
                                        messageId = DebugCommands.CMD_MOVE_HALT.ordinal
                                        robotId = -1 // target all
                                    }.build()
                                    CONNECTION_MANAGER.dispatchCommand(cmd)
                                }
                            }
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
