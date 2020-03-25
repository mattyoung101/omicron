package com.omicron.omicontrol.views

import RemoteDebug
import com.omicron.omicontrol.*
import com.omicron.omicontrol.field.FieldObject
import com.omicron.omicontrol.field.Robot
import javafx.geometry.Point2D
import javafx.geometry.Pos
import javafx.scene.canvas.GraphicsContext
import javafx.scene.control.Alert
import javafx.scene.control.Label
import javafx.scene.image.Image
import javafx.scene.input.KeyCode
import javafx.scene.input.KeyCodeCombination
import javafx.scene.input.KeyCombination
import javafx.scene.layout.Priority
import javafx.scene.paint.Color
import org.apache.commons.io.FileUtils
import org.greenrobot.eventbus.Subscribe
import org.tinylog.kotlin.Logger
import tornadofx.*
import kotlin.math.cos
import kotlin.math.roundToInt
import kotlin.math.sin
import kotlin.system.exitProcess


/**
 * This screen displays the localised positions of the robots on a virtual field and allows you to control them
 */
class FieldView : View() {
    private lateinit var display: GraphicsContext
    private lateinit var bandwidthLabel: Label
    private lateinit var selectedRobotLabel: Label
    private lateinit var ballLabel: Label
    private val fieldImage = Image("field_cropped_scaled.png")
    private val targetIcon = Image("target.png")
    private val robotImage = Image("robot.png")
    private val robotUnknown = Image("robot_unknown2.png")
    private val robotSelected = Image("robot_selected.png")
    private lateinit var localiserPerfLabel: Label
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
    private val ball = FieldObject()
    private val yellowGoal = FieldObject()
    private val blueGoal = FieldObject()
    private var bandwidthRecordingBegin = System.currentTimeMillis()
    private var isRaycastDebug = false
    private var isOptimiserDebug = false
    private var isIgnorePosition = false
    private var isReduceTransparency = false
    private var lastGoodPointList = listOf<RemoteDebug.RDPointF>()

    init {
        reloadStylesheetsOnFocus()
        title = "Field View | Omicontrol v${OMICONTROL_VERSION}"
    }

    // FIXME display localiser status again

    @ExperimentalUnsignedTypes
    @Subscribe
    fun receiveMessageEvent(message: RemoteDebug.DebugFrame) {
        BANDWIDTH += message.serializedSize
        val connectedRobotId = 0

        // this entire method is some really ugly code, sorry about that one, but it won't be fixed any time soon
        // well tbf the whole of Omicontrol is bad code which makes me sad but there's too much to refactor now I think

        runLater {
            // update data
            for (robot in robots){
                // assume unknown first, update later if true
                robot.isPositionKnown = false
            }
            for ((i, robot) in message.robotPositionsList.take(message.robotPositionsCount).withIndex()){
                if (!isIgnorePosition){
                    robots[i].position = Point2D(robot.x.toDouble(), robot.y.toDouble())
                } else {
                    robots[i].position = Point2D(0.0, 0.0)
                }
                // robots[i].orientation = message.robotOrientationsList[i] // FIXME broken right now
                robots[i].isPositionKnown = true
                robots[i].positionLabel?.text = String.format("Position: (%.2f, %.2f), %.2f", robot.x, robot.y, robots[i].orientation)
            }
            // update positions
            ball.isPositionKnown = message.isBallKnown
            if (ball.isPositionKnown)
                ball.position = Point2D(message.ballPos.x.toDouble(), message.ballPos.y.toDouble())
            yellowGoal.isPositionKnown = message.isYellowKnown
            if (yellowGoal.isPositionKnown)
                yellowGoal.position = Point2D(message.yellowGoalPos.x.toDouble(), message.yellowGoalPos.y.toDouble())
            blueGoal.isPositionKnown = message.isBlueKnown
            if (blueGoal.isPositionKnown)
                blueGoal.position = Point2D(message.blueGoalPos.x.toDouble(), message.blueGoalPos.y.toDouble())

            // update labels
            if (message.localiserRate > 0) {
                localiserPerfLabel.text = "${message.localiserRate} Hz (${1000 / message.localiserRate} ms), " +
                        "${message.localiserEvals} evals"
            }
            ballLabel.text = if (ball.isPositionKnown) String.format("(%.2f, %.2f)", ball.position.x, ball.position.y) else "Unknown"
            localiserPerfLabel.textFill = if (message.localiserStatus != "FTOL_REACHED") Color.ORANGE else Color.WHITE

            // draw the field
            display.fill = Color.BLACK
            display.fillRect(0.0, 0.0, FIELD_CANVAS_WIDTH, FIELD_CANVAS_HEIGHT)
            display.drawImage(fieldImage, 0.0, 0.0, FIELD_CANVAS_WIDTH, FIELD_CANVAS_HEIGHT)

            // render robots
            for (robot in robots) {
                val half = ROBOT_CANVAS_DIAMETER / 2.0
                val pos = robot.position.toCanvasPosition()
                val sprite = if (selectedRobot == robot) robotSelected else if (robot.isPositionKnown) robotImage else robotUnknown

                display.globalAlpha = if (isReduceTransparency) 0.3 else 1.0
                display.drawImage(sprite, pos.x - half, pos.y - half, ROBOT_CANVAS_DIAMETER, ROBOT_CANVAS_DIAMETER)
                display.globalAlpha = 1.0

                display.fill = Color.MAGENTA
                display.fillText(robot.id.toString(), pos.x - (half / 2.0), pos.y + (half / 2.0))
            }

            // render estimate stuff
            if (message.isBlueKnown && message.isYellowKnown) {
                display.fill = Color.FUCHSIA
                val pos = Point2D(message.goalEstimate.x.toDouble(), message.goalEstimate.y.toDouble()).toCanvasPosition()
                display.fillOval(pos.x, pos.y, 16.0, 16.0)
                display.fill = Color.WHITE
                display.fillText("est", pos.x, pos.y)

                display.stroke = Color.RED
                display.lineWidth = 3.0
                val bottomCorner = Point2D(message.estimateMinBounds.x.toDouble(), message.estimateMinBounds.y.toDouble()).toCanvasPosition()
                val topCorner = Point2D(message.estimateMaxBounds.x.toDouble(), message.estimateMaxBounds.y.toDouble()).toCanvasPosition()
                display.strokeRect(bottomCorner.x, bottomCorner.y, topCorner.x - bottomCorner.x, topCorner.y - bottomCorner.y)
            }

            // render ball
            run {
                display.fill = if (ball.isPositionKnown) Color.ORANGE else Color.GREY
                val pos = ball.position.toCanvasPosition()
                val half = BALL_CANVAS_DIAMETER / 2.0
                display.fillOval(pos.x - half, pos.y - half, BALL_CANVAS_DIAMETER, BALL_CANVAS_DIAMETER)
            }

            // render goals
            display.lineWidth = 4.0
            val centrePos = robots[connectedRobotId].position.toCanvasPosition()

            run {
                display.stroke = if (blueGoal.isPositionKnown) Color.YELLOW else Color.GREY
                val pos = yellowGoal.position.toCanvasPosition()
                display.strokeLine(centrePos.x, centrePos.y, pos.x, pos.y)
            }

            run {
                display.stroke = if (blueGoal.isPositionKnown) Color.DODGERBLUE else Color.GREY
                val pos = blueGoal.position.toCanvasPosition()
                display.strokeLine(centrePos.x, centrePos.y, pos.x, pos.y)
            }

            // render target sprite
            if (targetPos != null) {
                display.drawImage(targetIcon, targetPos!!.x - 16, targetPos!!.y - 16, 48.0, 48.0)
            }

            // draw rays if requested
            // TODO instead of picking robot 0, we need to pick the one we're connected to, to debug
            if (isRaycastDebug && robots[connectedRobotId].isPositionKnown) {
                // find min and max ray sscore
                val rayScores = message.rayScoresList.take(message.rayScoresCount)
                val minRayScore = rayScores.min()!!
                val maxRayScore = rayScores.max()!!

                display.lineWidth = 2.0
                display.stroke = Color.WHITE

                val original = Point2D(robots[connectedRobotId].position.x, robots[connectedRobotId].position.y).toCanvasPosition()
                val x0 = original.x
                val y0 = original.y
                var angle = 0.0

                for ((i, rayLength) in message.dewarpedRaysList.take(message.dewarpedRaysCount).withIndex()) {
                    if (rayLength < 0) continue // TODO may want to draw a red line to indicate missed ray
                    val x1 = x0 + (toFieldLength(rayLength) * sin(angle))
                    val y1 = y0 + (toFieldLength(rayLength) * cos(angle))

                    val normalisedScore = (rayScores[i] - minRayScore) / (maxRayScore - minRayScore)
                    // note that we're going FROM GOOD COLOUR TO BAD COLOUR because 0 means high accuracy and 1 means low accuracy
                    display.stroke = lerpColour(Color.WHITE, Color.RED, normalisedScore)
                    display.strokeLine(x0, y0, x1, y1)
                    angle += message.rayInterval
                }
            }

            // workaround for a stupid fucking bug where the point list size will be zero for some reason
            if (message.localiserVisitedPointsCount != 0){
                lastGoodPointList = message.localiserVisitedPointsList.take(message.localiserVisitedPointsCount)
            }

            // render optimiser debug (the path the optimiser took to the minimum)
            if (isOptimiserDebug){
                var lastPoint: Point2D? = null
                for ((i, point) in lastGoodPointList.withIndex()) {
                    val progress = (i / lastGoodPointList.size.toDouble()) * 255.0

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
            if (!IS_DEBUG_MODE) Utils.showGenericAlert(
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
                checkmenuitem("Raycast debug (little buggy)"){
                    selectedProperty().addListener { _, _, value ->
                        isRaycastDebug = value
                    }
                }
                checkmenuitem("Optimiser debug (buggy)"){
                    selectedProperty().addListener { _, _, value ->
                        isOptimiserDebug = value
                    }
                }
                checkmenuitem("Ignore sent position"){
                    selectedProperty().addListener { _, _, value ->
                        isIgnorePosition = value
                    }
                }
                checkmenuitem("Reduce sprite transparency"){
                    selectedProperty().addListener { _, _, value ->
                        isReduceTransparency = value
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

                        // don't place target if no robot is selected
                        if (selectedRobot == null) return@setOnMouseClicked

                        targetPos = if (targetPos != null && targetPos!!.distance(fieldCanvasPos) <= 24.0){
                            // user clicked on the same position, deselect target
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
                            label("Optimiser perf:"){ addClass(Styles.boldLabel) }
                            localiserPerfLabel = label("Unknown")
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
                            ballLabel = label("Unknown")
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
                                        val orientationStr = Utils.showTextInputDialog("Range: 0-360 degrees",
                                            "Enter new orientation").trim()
                                        if (orientationStr == "") return@setOnAction

                                        val newOrientation = orientationStr.toFloatOrNull() ?: run {
                                            Utils.showGenericAlert(Alert.AlertType.ERROR,
                                                "$orientationStr is not a valid number.",
                                                "Invalid orientation entered.")
                                            return@setOnAction
                                        }

                                        val cmd = RemoteDebug.DebugCommand.newBuilder().apply {
                                            messageId = DebugCommands.CMD_MOVE_ORIENT.ordinal
                                            robotId = selectedRobot!!.id
                                            orientation = newOrientation % 360.0f
                                        }.build()
                                        CONNECTION_MANAGER.dispatchCommand(cmd)
                                    }
                                }
                            }

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
