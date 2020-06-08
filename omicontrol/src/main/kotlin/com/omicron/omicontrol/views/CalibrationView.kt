package com.omicron.omicontrol.views

import RemoteDebug
import com.omicron.omicontrol.*
import com.omicron.omicontrol.maths.ModelApproach
import com.omicron.omicontrol.maths.PolynomialFitApproach
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
import javafx.stage.FileChooser
import net.objecthunter.exp4j.Expression
import net.objecthunter.exp4j.ExpressionBuilder
import org.apache.commons.math3.fitting.WeightedObservedPoints
import org.greenrobot.eventbus.Subscribe
import org.tinylog.kotlin.Logger
import tornadofx.*
import java.io.ByteArrayInputStream
import java.io.FileWriter
import java.nio.file.Paths
import kotlin.system.exitProcess

data class DewarpPoint(var pixelDistance: Double, var realDistance: Double = 0.0)

/**
 * This is essentially a stripped down version of the ConnectView which can be used to make calibrating the
 * camera dewarp model easier.
 */
class CalibrationView : View() {
    private lateinit var display: GraphicsContext
    private val measurements = mutableListOf<DewarpPoint>().observable()
    /** begin point of line or null if no line being drawn **/
    private var origin: Point2D? = null
    /** end point of line or null if no line being drawn **/
    private var end: Point2D? = null
    /** whether or not the user is currently drawing a line **/
    private var selecting = false
    private var mousePos = Point2D(0.0, 0.0)
    private var latestImage: Image? = null
    private var cropRect: RemoteDebug.RDRect? = null
    private lateinit var modelStatusLabel: Label
    /** whether the model has been loaded yet **/
    private var loadedModel = false
        set(value) {
            field = value
            modelStatusLabel.text = "Model status: ${if(value) "Loaded" else "Not loaded"}"
        }
    private var ghostPoints = mutableListOf<Point2D>()
    /** loaded or calculated mirror model */
    private var model: Expression? = null
    /** if true, snap initial point to centre of screen */
    private var snapInitialPoint = true
    /** if true, automatically add 5 centimetres (TODO configurable) to centimetres column */
    private var isAutoIncrement = true
    /** counter for centimetres value when using auto increment */
    private var counter = 5.0
    private val modelCalculator: ModelApproach = PolynomialFitApproach(3)

    init {
        reloadStylesheetsOnFocus()
        title = "Calibration View | Omicontrol v${OMICONTROL_VERSION}"
    }

    private fun updateDisplay(){
        display.fill = Color.BLACK
        display.lineWidth = 4.0
        display.stroke = Color.RED
        display.fillRect(0.0, 0.0, CANVAS_WIDTH, CANVAS_HEIGHT)
        if (latestImage != null && cropRect != null) {
            // since Kotlin's automatic null cast checking is useless as fuck, we have to null assert these all manually
            display.drawImage(latestImage, cropRect!!.x.toDouble(), cropRect!!.y.toDouble(), latestImage!!.width, latestImage!!.height)
        }

        if (snapInitialPoint && origin != null){
            // if we're in snap initial point mode, always draw a line from origin to cursor if possible
            display.strokeLine(origin!!.x, origin!!.y, mousePos.x, mousePos.y)
        } else if (!snapInitialPoint && origin != null && end != null){
            // otherwise, if we're in normal mode and we have both points, keep the line between them drawn
            display.strokeLine(origin!!.x, origin!!.y, end!!.x, end!!.y)
        } else if (!snapInitialPoint && origin != null){
            // finally, try and draw a line between the origin and mouse
            display.strokeLine(origin!!.x, origin!!.y, mousePos.x, mousePos.y)
        }

        // draw the origin point if we have it
        if (origin != null){
            display.fill = Color.RED
            display.fillOval(origin!!.x - 5.0, origin!!.y - 5.0, 10.0, 10.0)
        }

        // draw ghost points, which show the endpoint of all lines
        for (point in ghostPoints){
            display.fill = Color.ORANGE
            display.fillOval(point.x - 5.0, point.y - 5.0, 10.0, 10.0)
        }

        // if we got no points, don't bother drawing anything
        if (!selecting && end == null) return
        val endPoint = if (selecting) mousePos else end!!

        // draw end point
        display.fill = Color.RED
        display.fillOval(endPoint.x - 5.0, endPoint.y - 5.0, 10.0, 10.0)

        // draw line distance text
        display.fill = Color.WHITE
        val posX = (origin!!.x + endPoint.x) / 2.0
        val posY = (origin!!.y + endPoint.y) / 2.0
        val dist = origin!!.distance(endPoint)

        if (loadedModel){
            val cmDist = model!!.setVariable("x", dist).evaluate()
            display.fillText(String.format("%.2f pixels (%.2f cm)", dist, cmDist), posX, posY)
        } else {
            display.fillText(String.format("%.2f pixels", dist), posX, posY)
        }
    }

    @ExperimentalUnsignedTypes
    @Subscribe
    fun receiveMessageEvent(message: RemoteDebug.DebugFrame) {
        // decode the message, but in this case we only care about the image
        val img = Image(ByteArrayInputStream(message.defaultImage.toByteArray()))
        latestImage = img
        cropRect = message.cropRect

        runLater {
            updateDisplay()
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
            disconnect()
        }
    }

    private fun disconnect(){
        Logger.debug("Disconnecting...")
        CONNECTION_MANAGER.disconnect()
        EVENT_BUS.unregister(this@CalibrationView)
        Utils.transitionMetro(this@CalibrationView, ConnectView())
    }

    private fun updateModel(function: String){
        // parse the expression
        Logger.debug("Setting model to: $function")

        try {
            // evaluate a simple test case to make sure it actually works
            val tmpModel = ExpressionBuilder(function).variables("x").build()
            tmpModel.setVariable("x", 1.0).evaluate()
            model = tmpModel
            loadedModel = true
            ghostPoints.clear()
            Logger.debug("Model seems to be valid")
        } catch (e: Exception){
            Logger.error("Model appears to be invalid!")
            Logger.error(e)
            Utils.showGenericAlert(Alert.AlertType.ERROR, "Details: ${e.message}\n\nPlease check function arguments and" +
                    " syntax are correct.", "Model \"$function\" appears to be invalid.")
            return
        }
    }

    override val root = vbox {
        setPrefSize(1600.0, 900.0)
        EVENT_BUS.register(this@CalibrationView)
        setSendFrames(true)

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
            menu("Actions"){
                item("Export as CSV"){
                    setOnAction {
                        val chooser = FileChooser().apply {
                            initialDirectory = Paths.get(".").toFile()
                            extensionFilters.add(FileChooser.ExtensionFilter("CSV file", "*.csv"))
                        }
                        val csvFile = chooser.showSaveDialog(currentWindow) ?: return@setOnAction

                        val writer = FileWriter(csvFile)
                        writer.append("Pixel dist,Real dist").appendln()
                        for (row in measurements){
                            writer.append("${row.pixelDistance},${row.realDistance}").appendln()
                        }
                        writer.close()
                        Utils.showGenericAlert(Alert.AlertType.INFORMATION,
                            "Saved to: $csvFile", "Export completed successfully.")
                    }
                }
            }
            menu("Settings"){
                checkmenuitem("Snap initial point to centre"){
                    isSelected = true
                    selectedProperty().addListener { _, _, newValue ->
                        snapInitialPoint = newValue
                    }
                }
                checkmenuitem("Auto increment"){
                    isSelected = true
                    selectedProperty().addListener { _, _, newValue ->
                        isAutoIncrement = newValue
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
                canvas(CANVAS_WIDTH, CANVAS_HEIGHT){
                    display = graphicsContext2D

                    setOnMouseClicked {
                        if (!selecting){
                            // first click, put down point
                            println("SELECTING")
                            origin = if (snapInitialPoint && cropRect != null){
                                val centreX = cropRect!!.x + (cropRect!!.width / 2.0)
                                val centreY = cropRect!!.y + (cropRect!!.height / 2.0)
                                Point2D(centreX, centreY)
                            } else {
                                Point2D(it.x, it.y)
                            }
                            selecting = true
                            end = null
                        } else {
                            // in snap initial point mode, we are always selecting
                            if (!snapInitialPoint) selecting = false
                            end = Point2D(it.x, it.y)

                            // if we haven't yet loaded a model, add it to the point list
                            if (!loadedModel) {
                                if (isAutoIncrement){
                                    // auto increment is on: add to list, then increment counter
                                    measurements.add(DewarpPoint(origin!!.distance(end), counter))
                                    counter += 5
                                } else {
                                    // auto increment is off, just add to list
                                    measurements.add(DewarpPoint(origin!!.distance(end)))
                                }

                                ghostPoints.add(end!!)
                            }
                        }
                        updateDisplay()
                    }

                    setOnMouseMoved {
                        mousePos = Point2D(it.x, it.y)
                        updateDisplay()
                    }
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
                            tableview(measurements){
                                column("Pixel dist", DewarpPoint::pixelDistance).makeEditable().contentWidth(16.0)
                                column("Real dist (cm)", DewarpPoint::realDistance).makeEditable().remainingWidth()

                                enableCellEditing()
                                regainFocusAfterEdit()

                                setOnKeyPressed {
                                    if (it.code == KeyCode.DELETE){
                                        items.remove(selectionModel.selectedItem)
                                    }
                                }
                                columnResizePolicy = SmartResize.POLICY
                            }
                        }
                    }

                    fieldset {
                        field {
                            button("Clear"){
                                setOnAction {
                                    measurements.clear()
                                    // in snap initial point mode, we are always selecting and always have same origin
                                    if (!snapInitialPoint){
                                        selecting = false
                                        origin = null
                                    }
                                    end = null
                                    loadedModel = false
                                    ghostPoints.clear()
                                    counter = 5.0
                                }
                            }
                        }

                        field {
                            button("Calculate model"){
                                setOnAction {
                                    Logger.info("Calculating model from data points")

                                    val dataPoints = WeightedObservedPoints()
                                    println("Data points:")
                                    for (point in measurements){
                                        // x axis is pixel distance, y axis is centimetre distance
                                        dataPoints.add(point.pixelDistance, point.realDistance)
                                        println("${point.pixelDistance},${point.realDistance}")
                                    }

                                    val coefficients = modelCalculator.calculateModel(dataPoints.toList())
                                    Logger.info("Calculated coefficients: ${coefficients.joinToString(",")}")
                                    Logger.info("Model function: ${modelCalculator.formatFunction(coefficients)}")

                                    // TODO display a popup here (or an alternate view) with a graph
                                }
                            }
                        }

                        field {
                            button("Load model"){
                                setOnAction {
                                    val funcStr = Utils.showTextInputDialog("", "Enter model function").trim()
                                    if (funcStr == "") return@setOnAction
                                    updateModel(funcStr)
                                }
                            }
                        }

                        field {
                            modelStatusLabel = label("Model status: Not loaded")
                        }
                    }
                }

                hgrow = Priority.ALWAYS
                vgrow = Priority.ALWAYS
                alignment = Pos.TOP_LEFT
            }

            hgrow = Priority.ALWAYS
        }

        vbox {
            paddingTop = 25.0
            alignment = Pos.CENTER
            button("Switch to camera view"){

                setOnAction {
                    Logger.debug("Changing views to CameraView via button")
                    EVENT_BUS.unregister(this@CalibrationView)
                    Utils.transitionMetro(this@CalibrationView, CameraView())
                }
            }
        }
    }
}
