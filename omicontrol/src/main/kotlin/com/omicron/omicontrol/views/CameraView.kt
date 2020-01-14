package com.omicron.omicontrol.views

import com.google.common.eventbus.Subscribe
import com.omicron.omicontrol.*
import javafx.geometry.Pos
import javafx.scene.control.Alert
import javafx.scene.image.Image
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
import javafx.scene.control.ComboBox
import javafx.scene.control.Label
import javafx.scene.control.Slider
import javafx.scene.image.WritableImage
import javafx.scene.layout.Priority
import javafx.scene.paint.Color
import org.tinylog.kotlin.Logger
import java.util.*
import kotlin.concurrent.fixedRateTimer
import kotlin.concurrent.thread

class CameraView : View() {
    /** displays the thresh image(s) **/
    private lateinit var display: GraphicsContext
    private lateinit var temperatureLabel: Label
    private lateinit var selectBox: ComboBox<String>
    private lateinit var lastFpsLabel: Label
    private lateinit var sizeLabel: Label
    private val compressor = Inflater()
    private var hideCameraFrame = false
    /** mapping between each field object and its threshold as received from Omicam **/
    private val objectThresholds = hashMapOf<FieldObjects, RemoteDebug.RDThreshold>()
    /** mapping between a name, eg "RMin" and an associated JavaFX slider. Stored in insertion order. **/
    private val sliders = linkedMapOf<String, Slider>()
    private var selectedObject = FieldObjects.OBJ_NONE
    /** token for threads to wait on until updateThresholdValues() completes **/
    private val newThreshReceivedToken = Object()
    private var grabSendTimer: Timer? = null

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
            val temp = message.temperature
            temperatureLabel.text = "${String.format("%.2f", temp)}°C"
            if (temp >= 75.0){
                temperatureLabel.textFill = Color.RED
            } else {
                temperatureLabel.textFill = Color.WHITE
            }
            lastFpsLabel.text = "Frame rate: ${message.fps} FPS"
            sizeLabel.text = "Packet size: ${message.serializedSize / 1024} KiB"

            // clear the canvas and display the normal camera frame
            display.fill = Color.BLACK
            display.fillRect(0.0, 0.0, CANVAS_WIDTH, CANVAS_HEIGHT)
            if (!hideCameraFrame) display.drawImage(img, message.cropRect.x.toDouble(), message.cropRect.y.toDouble(), img.width, img.height)

            if (selectedObject != FieldObjects.OBJ_NONE) {
                // write the received image data to the canvas, skipping black pixels (this fakes blend mode which won't work
                // for some reason)
                val tmpImage = WritableImage(CANVAS_WIDTH.toInt(), CANVAS_HEIGHT.toInt())
                for (i in 0 until bytes) {
                    val byte = outBuf[i].toUByte().toInt()
                    if (byte == 0) continue

                    val x = i % message.frameWidth
                    val y = i / message.frameWidth
                    tmpImage.pixelWriter.setColor(x, y, Color.MAGENTA)
                }
                display.drawImage(tmpImage, message.cropRect.x.toDouble(), message.cropRect.y.toDouble(), tmpImage.width, tmpImage.height)

                // draw threshold centre
                display.fill = Color.RED
                val objectX = message.ballCentroid.x.toDouble()
                val objectY = message.ballCentroid.y.toDouble()
                // hack alert: in Omicam, if the centroid doesn't exist we set it to 0.0 so don't draw it if it's 0 here
                // in future we should add a field called "exists" to the object and the rect so we only render them if they exist
                // in case the object centroid is actually at (0, 0) for some reason
                 if (objectX != 0.0 && objectY != 0.0)
                     display.fillOval(objectX, objectY, 10.0, 10.0)

                // draw threshold bounding box
                display.stroke = Color.RED
                display.lineWidth = 4.0
                if (message.ballRect.x != 0 && message.ballRect.y != 0) {
                    val x = message.ballRect.x.toDouble()
                    val y = message.ballRect.y.toDouble()
                    display.strokeRect(x, y, message.ballRect.width.toDouble(), message.ballRect.height.toDouble())
                }
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

    /** Pulls the latest set of thresholds from Omicam. Unlocks the thresh received token when done. **/
    private fun updateThresholdValues(){
        val command = RemoteDebug.DebugCommand
            .newBuilder()
            .setMessageId(DebugCommands.CMD_THRESHOLDS_GET_ALL.ordinal)
            .build()

        CONNECTION_MANAGER.dispatchCommand(command, {
            for ((i, thresh) in it.allThresholdsList.withIndex()){
                Logger.trace("Object ${FieldObjects.values()[i]} min: ${thresh.minList}, max: ${thresh.maxList}")
                objectThresholds[FieldObjects.values()[i]] = thresh
            }

            // notify awaiting threads that the new data has been received
            synchronized(newThreshReceivedToken) { newThreshReceivedToken.notifyAll() }
        }, {
            Logger.warn("Failed to update threshold values")
            Utils.showGenericAlert(Alert.AlertType.ERROR, "An error occurred updating the latest threshold values." +
                    "\nIf this error continues to happen, Omicontrol will no longer function properly." +
                    "\nPlease restart Omicam and Omicontrol and try again.", "Error updating threshold values")
            synchronized(newThreshReceivedToken) { newThreshReceivedToken.notifyAll() }
        })
    }

    /** Method which handles the logic for selecting a new field object including waiting, threading, etc **/
    private fun selectNewObject(newId: Int){
        val newObj = FieldObjects.values()[newId]
        selectBox.isDisable = true
        Logger.debug("Selecting new field object: $newObj")

        thread(name="selectNewObject() Waiter") {
            // update the current threshold values before switching to the new object
            // the reason for this crazy threading setup is to prevent race conditions and stuff which we had previously
            updateThresholdValues()
            Logger.trace("Invoked updateThresholdValues(), awaiting...")
            synchronized (newThreshReceivedToken) { newThreshReceivedToken.wait(2500) }
            Logger.trace("Finalised new thresholds!")

            val newObjThresh = objectThresholds[newObj] ?: run {
                // this would be weird and shouldn't happen, but let's log it just in case
                Logger.error("No threshold for $newObj?")
                return@thread
            }

            // tell Omicam to switch objects
            val msg = RemoteDebug.DebugCommand.newBuilder()
                .setMessageId(DebugCommands.CMD_THRESHOLDS_SELECT.ordinal)
                .setObjectId(newObj.ordinal)
                .build()
            Logger.trace("Telling Omicam to switch to selected object...")

            CONNECTION_MANAGER.dispatchCommand(msg, {
                selectedObject = newObj
                // re-enable sliders if the object selected is not OBJ_NONE
                for (slider in sliders) {
                    slider.value.isDisable = selectedObject == FieldObjects.OBJ_NONE
                }

                // set min then max slider values
                for ((i, colour) in COLOURS.withIndex()) {
                    sliders["${colour}Min"]?.value = newObjThresh.minList[i].toDouble()
                }
                for ((i, colour) in COLOURS.withIndex()) {
                    sliders["${colour}Max"]?.value = newObjThresh.maxList[i].toDouble()
                }

                selectBox.isDisable = false
            }, {
                selectBox.isDisable = false
            })
        }
    }

    /** sends threshold slider value change to Omicontrol **/
    private fun publishSliderValueChange(colour: String, type: String, value: Int){
        // Logger.trace("Slider $colour$type changed to new value: $value")
        val msg = RemoteDebug.DebugCommand.newBuilder().apply {
            messageId = DebugCommands.CMD_THRESHOLDS_SET.ordinal
            minMax = type.toLowerCase() == "min"
            colourChannel = COLOURS.indexOf(colour.toUpperCase())
            this.value = value
        }.build()
        CONNECTION_MANAGER.dispatchCommand(command=msg, ignoreErrors=true)
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

                // when the mouse is pressed start the drag timer
                setOnMousePressed {
                    grabSendTimer = fixedRateTimer(name="GrabSendTimer", period=GRAB_SEND_TIMER_PERIOD, action = {
                        publishSliderValueChange(colour, type, value.toInt())
                    })
                }
                // when the mouse is finished being moved, cancel the grab timer and send the request to Omicam
                setOnMouseReleased {
                    grabSendTimer?.cancel()
                    grabSendTimer?.purge()
                    publishSliderValueChange(colour, type, value.toInt())
                }
                // left and right arrow keys
                setOnKeyReleased {
                    if (it.code == KeyCode.LEFT || it.code == KeyCode.RIGHT)
                        publishSliderValueChange(colour, type, value.toInt())
                }
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
                        colourSlider.value = newValue.toDouble()
                        publishSliderValueChange(colour, type, newValue)
                    }
                }
            }
        }
    }

    private fun disconnect(){
        CONNECTION_MANAGER.disconnect()
        EVENT_BUS.unregister(this)
        // if the user is stupid enough to press CTRL+D WHILE dragging the slider, stop them
        grabSendTimer?.cancel()
        grabSendTimer?.purge()
        Utils.transitionMetro(this@CameraView, ConnectView())
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
                        disconnect()
                    }
                    accelerator = KeyCodeCombination(KeyCode.D, KeyCombination.CONTROL_DOWN)
                }
            }
            menu("Actions") {
                item("Reboot camera"){
                    setOnAction {
                        if (Utils.showConfirmDialog("This may take some time.", "Reboot the SBC now?")){
                            val msg = RemoteDebug.DebugCommand.newBuilder()
                                .setMessageId(DebugCommands.CMD_POWER_REBOOT.ordinal)
                                .build()
                            CONNECTION_MANAGER.dispatchCommand(msg, {
                                Logger.info("Reboot command sent successfully, disconnecting")
                                disconnect()
                            })
                        }
                    }
                }
                item("Shutdown camera"){
                    setOnAction {
                        if (Utils.showConfirmDialog("You will need to power it back on manually.", "Shutdown the SBC now?")){
                            val msg = RemoteDebug.DebugCommand.newBuilder()
                                .setMessageId(DebugCommands.CMD_POWER_OFF.ordinal)
                                .build()
                            CONNECTION_MANAGER.dispatchCommand(msg, {
                                Logger.info("Shutdown command sent successfully, disconnecting")
                                disconnect()
                            })
                        }
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
                            Alert.AlertType.INFORMATION, "Your settings have been saved permanently to the remote host.",
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
                canvas(CANVAS_WIDTH, CANVAS_HEIGHT){
                    display = graphicsContext2D
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

                form {
                    // RGB colour sliders
                    for (colour in COLOURS){
                        fieldset {
                            add(generateSlider(colour, "Min"))
                            add(generateSlider(colour, "Max"))
                        }
                    }

                    // object selection dropdown
                    fieldset {
                        field {
                            label("Select object: ")
                            selectBox = combobox<String> {
                                items = FXCollections.observableArrayList(FieldObjects.values().map { it.toString() })
                                selectionModel.selectFirst()

                                // listen for changes to the slider
                                valueProperty().addListener { _, _, _ ->
                                    selectNewObject(selectionModel.selectedIndex)
                                }
                            }
                        }
                    }

                    fieldset {
                        field {
                            label("Hide camera frame: ")
                            checkbox {
                                selectedProperty().addListener { _, _, newValue ->
                                    hideCameraFrame = newValue
                                }
                            }
                        }
                    }

                    fieldset {
                        field {
                            lastFpsLabel = label("Last FPS: None recorded")
                        }
                    }

                    fieldset {
                        field {
                            label("Temperature: ")
                            temperatureLabel = label("Unknown")
                        }
                    }

                    fieldset {
                        field {
                            lastPingLabel = label("Last ping: None recorded")
                        }
                    }

                    fieldset {
                        field {
                            sizeLabel = label("Packet size: None recorded")
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
            display.fillRect(0.0, 0.0, CANVAS_WIDTH, CANVAS_HEIGHT)
        }
    }
}