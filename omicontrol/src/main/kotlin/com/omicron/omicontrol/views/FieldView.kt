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
import javafx.scene.shape.Circle
import org.apache.commons.io.FileUtils
import kotlin.math.cos
import kotlin.math.roundToInt
import kotlin.math.sin

/**
 * This screen displays the localised positions of the robots on a virtual field and allows you to control them
 */
class FieldView : View() {
    private lateinit var display: GraphicsContext
    private lateinit var bandwidthLabel: Label
    private lateinit var selectedRobotLabel: Label
    private val fieldImage = Image("field_cropped_scaled.png")
    private val targetIcon = Image("target.png")
    private val robotImage = Image("robot.png")
    private val robotUnknown = Image("robot_unknown.png")
    private val robotSelected = Image("robot_selected.png")
    private val robots = listOf(
        Robot(0),
        Robot(1)
    )
    private var selectedRobot: Robot? = null
        set(value) {
            selectedRobotLabel.text = value?.id?.toString() ?: "None"
            field = value
        }
    private var targetPos: Point2D? = null
    private val ball = Ball()
    private var bandwidthRecordingBegin = System.currentTimeMillis()
    private var isRaycastDebug = false
    private var isOptimiserDebug = false

    init {
        reloadStylesheetsOnFocus()
        title = "Field View | Omicontrol"
    }

    @ExperimentalUnsignedTypes
    @Subscribe
    fun receiveMessageEvent(message: RemoteDebug.DebugFrame) {
        BANDWIDTH += message.serializedSize

        runLater {
            // update data
            for (robot in robots){
                // assume unknown first, update later if true
                robot.isPositionKnown = false
            }
            for ((i, robot) in message.robotPositionsList.take(message.robotPositionsCount).withIndex()){
                robots[i].position = Point2D(robot.x.toDouble(), robot.y.toDouble())
                // robots[i].orientation = message.robotOrientationsList[i] // FIXME broken right now
                robots[i].isPositionKnown = true
                robots[i].positionLabel?.text = String.format("Position: (%.2f, %.2f), %.2f", robot.x, robot.y, robots[i].orientation)
            }

            display.fill = Color.BLACK
            display.fillRect(0.0, 0.0, FIELD_CANVAS_WIDTH, FIELD_CANVAS_HEIGHT)
            display.drawImage(fieldImage, 0.0, 0.0, FIELD_CANVAS_WIDTH, FIELD_CANVAS_HEIGHT)

            // render robots
            for (robot in robots) {
                val half = ROBOT_CANVAS_DIAMETER / 2.0
                val pos = robot.position.toCanvasPosition()
                val sprite = if (selectedRobot == robot) robotSelected else if (robot.isPositionKnown) robotImage else robotUnknown
                display.drawImage(sprite, pos.x - half, pos.y - half, ROBOT_CANVAS_DIAMETER, ROBOT_CANVAS_DIAMETER)

                display.fill = Color.WHITE
                display.fillText(robot.id.toString(), pos.x - (half / 2.0), pos.y + (half / 2.0))
            }

            // render ball
            display.fill = if (ball.isPositionKnown) Color.ORANGE else Color.GREY
            val pos = ball.position.toCanvasPosition()
            val half = BALL_CANVAS_DIAMETER / 2.0
            display.fillOval(pos.x - half, pos.y - half, BALL_CANVAS_DIAMETER, BALL_CANVAS_DIAMETER)

            // render target
            if (targetPos != null) {
                display.drawImage(targetIcon, targetPos!!.x - 16, targetPos!!.y - 16, 48.0, 48.0)
            }

            // draw rays if requested
            // TODO instead of picking robot 0, we need to pick the one we're connected to, to debug
            val connectedRobot = 0
            if (isRaycastDebug && robots[connectedRobot].isPositionKnown) {
                display.lineWidth = 2.0
                display.stroke = Color.WHITE

                val original =
                    Point2D(robots[connectedRobot].position.x, robots[connectedRobot].position.y).toCanvasPosition()
                val x0 = original.x
                val y0 = original.y
                var angle = 0.0

                for (ray in message.dewarpedRaysList.take(message.dewarpedRaysCount)) {
                    val x1 = x0 + (toFieldLength(ray) * sin(angle))
                    val y1 = y0 + (toFieldLength(ray) * cos(angle))

                    display.strokeLine(x0, y0, x1, y1)
                    angle += message.rayInterval
                }
            }

            if (isOptimiserDebug){
                var lastPoint: Point2D? = null
                for ((i, point) in message.localiserVisitedPointsList.take(message.localiserVisitedPointsCount).withIndex()) {
                    val progress = (i / message.localiserVisitedPointsCount.toDouble()) * 255.0

                    val fieldPoint = Point2D(point.x.toDouble(), point.y.toDouble()).toCanvasPosition()
                    display.fill = Color.rgb(progress.roundToInt(), 0, 0)
                    display.fillOval(fieldPoint.x - 5.0, fieldPoint.y - 5.0, 10.0, 10.0)

                    if (lastPoint != null) {
                        // draw arrow
                        display.stroke = Color.rgb(progress.roundToInt(), 0, 0)
                        display.lineWidth = 2.0
                        display.strokeLine(lastPoint.x, lastPoint.y, fieldPoint.x, fieldPoint.y)
                    }
                    lastPoint = fieldPoint
                }
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
            menu("Settings"){
                checkmenuitem("Raycast debug"){
                    selectedProperty().addListener { _, _, value ->
                        isRaycastDebug = value
                    }
                }
                checkmenuitem("Optimiser debug"){
                    selectedProperty().addListener { _, _, value ->
                        isOptimiserDebug = value
                    }
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
                        Logger.info("Mouse clicked at real: $fieldRealPos, canvas: $fieldCanvasPos")

                        for (robot in robots.reversed()){
                            val half = ROBOT_CANVAS_DIAMETER / 2.0
                            val pos = robot.position.toCanvasPosition()

                            if (pointInCircle(fieldCanvasPos, pos, half)){
                                Logger.trace("Clicked on robot ${robot.id}")

                                selectedRobot = if (selectedRobot == robot){
                                    Logger.trace("Deselecting robot")
                                    null
                                } else {
                                    Logger.trace("Selecting robot")
                                    robot
                                }
                                return@setOnMouseClicked
                            }
                        }

                        targetPos = if (targetPos != null && targetPos!!.distance(fieldCanvasPos) <= 16.0){
                            // user clicked on the same position, deslect target
                            null
                        } else {
                            // new position to move target to
                            fieldCanvasPos
                        }

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
                            selectedRobotLabel = label("None")
                        }
                    }
                    fieldset {
                        field {
                            label("Robot 0:"){ addClass(Styles.boldLabel) }
                        }
                        field {
                            robots[0].positionLabel = label("Position: Unknown")
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
                            robots[1].positionLabel = label("Position: Unknown")
                        }
                        field {
                            label("FSM state: Unknown")
                        }
                    }

                    fieldset {
                        field {
                            label("Ball:"){ addClass(Styles.boldLabel) }
                        }
                        field {
                            // ball position will always be according to connected robot
                            label("Position:")
                            label("Unknown")
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
                                            robotId = selectedRobot!!.id
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
                                            robotId = selectedRobot!!.id
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
                                            robotId = selectedRobot!!.id
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
                                    Logger.debug("Changing views to CameraView via button")
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
