package com.omicron.omicontrol.views

import com.google.common.eventbus.Subscribe
import com.omicron.omicontrol.*
import javafx.geometry.Pos
import javafx.scene.control.Alert
import javafx.scene.image.Image
import javafx.scene.image.ImageView
import tornadofx.*
import java.io.ByteArrayInputStream
import kotlin.system.exitProcess
import javafx.scene.canvas.GraphicsContext
import javafx.scene.input.KeyCode
import javafx.scene.input.KeyCodeCombination
import javafx.scene.input.KeyCombination
import java.util.zip.Inflater
import RemoteDebug
import javafx.collections.FXCollections
import javafx.scene.Cursor
import javafx.scene.Node
import javafx.scene.control.Label
import javafx.scene.control.Slider
import javafx.scene.layout.Priority
import javafx.scene.paint.Color
import org.tinylog.kotlin.Logger
import kotlin.concurrent.thread
import kotlin.math.floor

class CameraView : View() {
    /** displays the normal image **/
    private lateinit var defaultImage: ImageView
    /** displays the thresh image(s) **/
    private lateinit var display: GraphicsContext
    private lateinit var temperatureLabel: Label
    private val compressor = Inflater()
    private var hideCameraFrame = false
    /** mapping between each field object and its threshold as received from Omicam **/
    private val objectThresholds = hashMapOf<FieldObjects, RemoteDebug.RDThreshold>()
    /** mapping between a name, eg "RMin" and an associated JavaFX slider. Stored in insertion order. **/
    private val sliders = linkedMapOf<String, Slider>()
    private var selectedObject = FieldObjects.OBJ_NONE
    /** token for threads to wait on until updateThresholdValues() completes **/
    private val newThreshReceivedToken = Object()

    init {
        reloadStylesheetsOnFocus()
        title = "Camera View | Omicontrol"
        EVENT_BUS.register(this)
    }

    @ExperimentalUnsignedTypes
    @Subscribe
    fun receiveMessageEvent(message: RemoteDebug.DebugFrame) {
        compressor.reset()
        compressor.setInput(message.ballThreshImage.toByteArray())
        val outBuf = ByteArray(1024 * 1024) // 1 megabyte (in binary bytes)
        val bytes = compressor.inflate(outBuf)

        val img = Image(ByteArrayInputStream(message.defaultImage.toByteArray()))

        runLater {
            val begin = System.currentTimeMillis()
            val temp = message.temperature
            temperatureLabel.text = "${String.format("%.2f", temp)}°C"
            if (temp >= 75.0){
                temperatureLabel.textFill = Color.RED
            } else {
                temperatureLabel.textFill = Color.WHITE
            }

            // clear the canvas and display the normal camera frame
            display.clearRect(0.0, 0.0, IMAGE_WIDTH, IMAGE_HEIGHT)
            if (!hideCameraFrame) display.drawImage(img, 0.0, 0.0, IMAGE_WIDTH, IMAGE_HEIGHT)

            // TODO add support for each object (not just the ball)
            if (selectedObject != FieldObjects.OBJ_NONE) {
                // write the received image data to the canvas, skipping black pixels (this fakes blend mode which won't work
                // for some reason)
                for (i in 0 until bytes) {
                    val byte = outBuf[i].toUByte().toInt()
                    if (byte == 0) continue

                    val x = i % IMAGE_WIDTH.toInt()
                    val y = floor(i / IMAGE_WIDTH).toInt()
                    display.pixelWriter.setColor(x, y, Color.ORANGE)
                }

                // draw ball centre
                display.fill = Color.RED
                val ballX = message.ballCentroid.x.toDouble()
                val ballY = message.ballCentroid.y.toDouble()
                display.fillOval(ballX, ballY, 10.0, 10.0)

                // draw ball bounding box
                display.stroke = Color.RED
                display.lineWidth = 4.0
                display.strokeRect(message.ballRect.x.toDouble(), message.ballRect.y.toDouble(),
                    message.ballRect.width.toDouble(), message.ballRect.height.toDouble())
            }
        }
    }

    @Subscribe
    fun receiveRemoteShutdownEvent(message: RemoteShutdownEvent){
        Logger.info("Received remote shutdown event")
        runLater {
            Utils.showGenericAlert(Alert.AlertType.ERROR, "The remote connection has unexpectedly terminated.\n" +
                    "Please check Omicam is still running and try again.\n\nError description: Protobuf message was null",
                "Connection error")
            Utils.transitionMetro(this, ConnectView())
        }
    }

    private fun updateThresholdValues(){
        val command = RemoteDebug.DebugCommand
            .newBuilder()
            .setMessageId(DebugCommands.CMD_THRESHOLDS_GET_ALL.ordinal)
            .build()

        CONNECTION_MANAGER.dispatchCommand(command, {
            for ((i, thresh) in it.allThresholdsList.withIndex()){
                println("Object ${FieldObjects.values()[i]} min: ${thresh.minList}, max: ${thresh.maxList}")
                objectThresholds[FieldObjects.values()[i]] = thresh
            }
            synchronized(newThreshReceivedToken) { newThreshReceivedToken.notifyAll() }
        }, { errMsg ->
            Logger.warn("Failed to update threshold values: $errMsg")
            Utils.showGenericAlert(Alert.AlertType.ERROR, "An error occurred updating the latest threshold values." +
                    "\nIf this error continues to happen, Omicontrol will no longer function properly." +
                    "\nPlease restart Omicam and Omicontrol and try again.", "Error updating threshold values")
            synchronized(newThreshReceivedToken) { newThreshReceivedToken.notifyAll() }
        })
    }

    private fun selectNewObject(newId: Int){
        val newObj = FieldObjects.values()[newId]
        Logger.debug("Selecting new field object: $newObj")

        thread(name="selectNewObject() Waiter") {
            // update the current threshold values before switching to the new object
            // the reason for this crazy threading setup is to prevent race conditions and stuff which we had previously
            updateThresholdValues()
            Logger.trace("Invoked updateThresholdValues(), awaiting...")
            synchronized (newThreshReceivedToken) { newThreshReceivedToken.wait(2500) }
            Logger.trace("Finalised new thresholds!")

            val newObjThresh = objectThresholds[newObj] ?: run {
                Logger.error("No threshold for $newObj?")
                return@thread
            }

            // tell Omicam to switch objects
            val msg = RemoteDebug.DebugCommand.newBuilder()
                .setMessageId(DebugCommands.CMD_THRESHOLDS_SELECT.ordinal)
                .setThresholdId(newObj.ordinal)
                .build()
            val colours = listOf("R", "G", "B")
            Logger.trace("Telling Omicam to switch to selected object...")

            CONNECTION_MANAGER.dispatchCommand(msg, {
                selectedObject = newObj
                // re-enable sliders if the object selected is not OBJ_NONE
                for (slider in sliders) {
                    slider.value.isDisable = selectedObject == FieldObjects.OBJ_NONE
                }

                // set min then max slider values
                for ((i, colour) in colours.withIndex()) {
                    sliders["${colour}Min"]?.value = newObjThresh.minList[i].toDouble()
                }
                for ((i, colour) in colours.withIndex()) {
                    sliders["${colour}Max"]?.value = newObjThresh.maxList[i].toDouble()
                }
            })
        }
    }

    /**
     * Generates a JavaFX field containing a slider and label for the given colour channel in the threshold
     * @param colour the name of the colour channel, eg "R"
     * @param type "min" or "max"
     **/
    private fun generateSlider(colour: String, type: String): Field {
        return field("$colour $type") {
            val colourSlider = slider(min=0, max=255){
                blockIncrement = 1.0
                majorTickUnit = 1.0
                minorTickCount = 0
                isSnapToTicks = true
                isDisable = true // since we start out in OBJ_NONE and you can't edit it
            }
            sliders["$colour$type"] = colourSlider

            label("000"){
                colourSlider.valueProperty().addListener { _, _, newValue ->
                    text = newValue.toInt().toString().padStart(3, '0')
                }
                cursor = Cursor.HAND

                // allow the user to manually input the threshold value
                setOnMouseClicked {
                    // you can't edit the none object threshold
                    if (selectedObject == FieldObjects.OBJ_NONE) return@setOnMouseClicked

                    val result = Utils.showTextInputDialog("Enter new value (0-255):", "Manual threshold input",
                        default = colourSlider.value.toInt().toString())
                    if (result.isBlank()) return@setOnMouseClicked

                    val newValue = result.toIntOrNull()
                    if (newValue == null || newValue > 255 || newValue < 0){
                        Utils.showGenericAlert(Alert.AlertType.ERROR, "\"$result\" is not a valid threshold.",
                            "Invalid input")
                    } else {
                        colourSlider.valueProperty().set(newValue.toDouble())
                    }
                }
            }
        }
    }

    override val root = vbox {
        setPrefSize(1600.0, 900.0)

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
                        CONNECTION_MANAGER.disconnect()
                        EVENT_BUS.unregister(this)
                        Utils.transitionMetro(this@CameraView, ConnectView())
                    }
                    accelerator = KeyCodeCombination(KeyCode.D, KeyCombination.CONTROL_DOWN)
                }
            }
            menu("Actions") {
                item("Reboot camera"){
                    setOnAction {
                        // TODO send reboot command
                    }
                }
                item("Shutdown camera"){
                    setOnAction {
                        // TODO send shutdown command
                    }
                }
                item("Save config"){
                    accelerator = KeyCodeCombination(KeyCode.S, KeyCombination.CONTROL_DOWN)
                    setOnAction {
                        val msg = RemoteDebug.DebugCommand.newBuilder()
                            .setMessageId(DebugCommands.CMD_THRESHOLDS_WRITE_DISK.ordinal)
                            .build()
                        CONNECTION_MANAGER.dispatchCommand(msg, {
                            Utils.showGenericAlert(
                            Alert.AlertType.INFORMATION, "Your settings have been saved to the remote host.",
                            "Config saved successfully"
                            )
                        })
                    }
                }
            }
            menu("Help") {
                item("What's this?").setOnAction {
                    Utils.showGenericAlert(Alert.AlertType.INFORMATION, """
                        In the camera view, you can see the output of the camera and tune new thresholds. Use the "Select
                        object" dropdown to select which object to view, then adjust the sliders. Select "None" to view
                        just the normal camera frame, and tick the "Hide camera frame" button to view just the thresholds.
                        
                        Only the coloured blobs (corresponding to the selected colour) are pixels included in the threshold.
                        
                        Pro Tip! Click on the number to the right of each threshold slider to type in its value manually.
                        
                        It's VERY IMPORTANT that you select Actions > Save Config (or CTRL+S) before quitting to write
                        your threshold values to the remote INI file.
                    """.trimIndent(), "Camera View Help")
                }
                item("About").setOnAction {
                    Utils.showGenericAlert(Alert.AlertType.INFORMATION, "Copyright (c) 2019 Team Omicron. See LICENSE.txt",
                        "Omicontrol v${OMICONTROL_VERSION}", "About")
                }
            }
        }

        hbox {
            vbox {
                stackpane {
                    defaultImage = imageview()

                    // real canvas which the frame buffer is copied to and downscaled
                    canvas(IMAGE_WIDTH, IMAGE_HEIGHT){
                        display = graphicsContext2D
                    }

                    val indicator = progressindicator {
                        scaleX = 3.0
                        scaleY = 3.0
                        isVisible = false
                    }
                    CONNECTION_MANAGER.progressIndicator = indicator
                }
                alignment = Pos.TOP_RIGHT
            }

            vbox {
                hbox {
                    label("Camera Configuration") {
                        addClass(Styles.bigLabel)
                        alignment = Pos.TOP_CENTER
                    }
                    hgrow = Priority.ALWAYS
                    alignment = Pos.TOP_CENTER
                }

                // TODO we're gonna need to get the initial values from the remote to populate the sliders with
                // TODO make all the sliders text fields so you can type in values if required (or maybe if you click on the label it lets you do that)

                val colours = listOf("R", "G", "B")
                form {
                    // RGB colour sliders
                    for (colour in colours){
                        fieldset {
                            add(generateSlider(colour, "Min"))
                            add(generateSlider(colour, "Max"))
                        }
                    }

                    // temperature display
                    fieldset {
                        field {
                            label("Temperature: ")
                            temperatureLabel = label("Unknown")
                        }
                    }

                    fieldset {
                        field {
                            label("Select object: ")
                            combobox<String> {
                                items = FXCollections.observableArrayList(FieldObjects.values().map { it.toString() })
                                selectionModel.selectFirst()

                                // listen for changes to the slider
                                valueProperty().addListener { _, _, _ ->
                                    selectNewObject(selectionModel.selectedIndex)
                                }
                            }
                        }
                    }

                    fieldset{
                        field {
                            label("Hide camera frame: ")
                            checkbox {
                                selectedProperty().addListener { _, _, newValue ->
                                    hideCameraFrame = newValue
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
            button("Switch to robot view")
        }

        if (DEBUG_CAMERA_VIEW){
            display.fill = Color.WHITE
            display.fillRect(0.0, 0.0, IMAGE_WIDTH, IMAGE_HEIGHT)
        }
    }
}