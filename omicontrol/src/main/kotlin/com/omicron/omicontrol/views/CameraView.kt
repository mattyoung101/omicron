package com.omicron.omicontrol.views

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
import com.github.ajalt.colormath.*
import javafx.collections.FXCollections
import javafx.scene.Cursor
import javafx.scene.control.ComboBox
import javafx.scene.control.Label
import javafx.scene.control.Slider
import javafx.scene.image.WritableImage
import javafx.scene.layout.Priority
import javafx.scene.layout.VBox
import javafx.scene.paint.Color
import javafx.stage.FileChooser
import org.apache.commons.io.FileUtils
import org.greenrobot.eventbus.Subscribe
import org.tinylog.kotlin.Logger
import java.io.FileOutputStream
import java.nio.file.Paths
import java.util.*
import java.util.concurrent.atomic.AtomicBoolean
import kotlin.concurrent.fixedRateTimer
import kotlin.concurrent.thread
import kotlin.math.cos
import kotlin.math.sin

class CameraView : View() {
    /** displays the thresh image(s) **/
    private lateinit var display: GraphicsContext
    private lateinit var temperatureLabel: Label
    private lateinit var selectBox: ComboBox<String>
    private lateinit var lastFpsLabel: Label
    private lateinit var bandwidthLabel: Label
    private val compressor = Inflater()
    private var hideCameraFrame = false
    private var localiserDebug = false
    /** mapping between each field object and its threshold as received from Omicam **/
    private val objectThresholds = hashMapOf<FieldObjects, RemoteDebug.RDThreshold>()
    /** mapping between a name, eg "RMin" and an associated JavaFX slider. Stored in insertion order. **/
    private val sliders = linkedMapOf<String, Slider>()
    private var selectedObject = FieldObjects.OBJ_NONE
    /** token for threads to wait on until updateThresholdValues() completes **/
    private val newThreshReceivedToken = Object()
    /** timer which goes off when the user drags a slider **/
    private var grabSendTimer: Timer? = null
    /** when the last second recording began in system time **/
    private var bandwidthRecordingBegin = System.currentTimeMillis()
    private var saveNextToDisk = false
    private var savingNextToDisk = AtomicBoolean()
    /** if true, when a min/max slider is dragged, it will update the value to all min/max sliders **/
    private var multiSlide = false
    private var colourSpace: ColourSpace = RGB
    private val minColour = IntArray(3)
    private val maxColour = IntArray(3)
    private lateinit var sliderVbox: VBox

    init {
        reloadStylesheetsOnFocus()
        title = "Camera View | Omicontrol v${OMICONTROL_VERSION}"
    }

    @ExperimentalUnsignedTypes
    @Subscribe
    fun receiveMessageEvent(message: RemoteDebug.DebugFrame) {
        // decode the message
        compressor.reset()
        compressor.setInput(message.ballThreshImage.toByteArray())
        val outBuf = ByteArray(1024 * 1024) // 1 megabyte (in binary bytes)
        val bytes = compressor.inflate(outBuf)
        val img = Image(ByteArrayInputStream(message.defaultImage.toByteArray()))

        // display megabits per second of bandwidth used by the app
        BANDWIDTH += message.serializedSize

        if (savingNextToDisk.get()){
            return
        }

        runLater {
            // handle save to disk
            if (saveNextToDisk) {
                savingNextToDisk.set(true)
                val chooser = FileChooser().apply {
                    initialDirectory = Paths.get(".").toFile()
                    extensionFilters.add(FileChooser.ExtensionFilter("JPEG images", "*.jpg"))
                }
                val imageOut = chooser.showSaveDialog(currentWindow)
                if (imageOut != null) {
                    FileOutputStream(imageOut).use {
                        // in theory, since we receive a full jpeg image, we should just be able to write it directly to disk
                        // and it should be able to be picked up correctly
                        it.write(message.defaultImage.toByteArray())
                    }
                    Utils.showGenericAlert(Alert.AlertType.INFORMATION, "Saved to: $imageOut",
                        "Saved image to disk successfully")
                    Logger.debug("Written frame to disk as $imageOut")
                }
                saveNextToDisk = false
                savingNextToDisk.set(false)
            }

            val temp = message.temperature
            temperatureLabel.text = "Temperature: ${String.format("%.2f", temp)}°C"
            when {
                temp <= 50 -> {
                    temperatureLabel.textFill = Color.LIME
                }
                temp.toInt() in 51..75 -> {
                    temperatureLabel.textFill = Color.ORANGE
                }
                temp > 75 -> {
                    temperatureLabel.textFill = Color.RED
                }
            }

            val fps = message.fps
            lastFpsLabel.text = "Frame rate: $fps FPS (${if (fps <= 0) "undefined" else (1000 / fps).toString()} ms)"
            when {
                fps >= 55 -> {
                    lastFpsLabel.textFill = Color.LIME
                }
                fps in 45..54 -> {
                    lastFpsLabel.textFill = Color.ORANGE
                }
                else -> {
                    lastFpsLabel.textFill = Color.RED
                }
            }

            if (System.currentTimeMillis() - bandwidthRecordingBegin >= 1000){
                bandwidthLabel.text = "Bandwidth: ${BANDWIDTH / 1024} Kb/s"
                bandwidthRecordingBegin = System.currentTimeMillis()
                BANDWIDTH = 0
            }

            // clear the canvas and display the normal camera frame
            display.fill = Color.BLACK
            display.fillRect(0.0, 0.0, CANVAS_WIDTH, CANVAS_HEIGHT)
            if (!hideCameraFrame) display.drawImage(img, message.cropRect.x.toDouble(), message.cropRect.y.toDouble(), img.width, img.height)

            if (selectedObject != FieldObjects.OBJ_NONE) {
                // write the received image data to the canvas, skipping black pixels (this fakes blend mode which won't work
                // for some reason)
                // we create a temporary image of the entire canvas size and use it like an FBO, kind of a hack
                val tmpImage = WritableImage(CANVAS_WIDTH.toInt(), CANVAS_HEIGHT.toInt())
                for (i in 0 until bytes) {
                    val byte = outBuf[i].toUByte().toInt()
                    if (byte == 0) continue

                    val x = i % message.frameWidth
                    val y = i / message.frameWidth
                    tmpImage.pixelWriter.setColor(x, y, Color.MAGENTA)
                }
                display.drawImage(tmpImage, message.cropRect.x.toDouble(), message.cropRect.y.toDouble(), tmpImage.width, tmpImage.height)

                // note that Omicam fixes object positioning when cropping/scaling is enabled on its side, so we don't have
                // to worry about it here, at least not for the centroid and bounding box

                // draw threshold centre
                display.fill = Color.RED
                val objectX = message.ballCentroid.x.toDouble()
                val objectY = message.ballCentroid.y.toDouble()
                // FIXME definitely need to add an "exists" parameter to the protobuf as this breaks when scaling is enabled
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

            // draw localiser debug
            if (localiserDebug) {
                display.lineWidth = 2.0
                display.stroke = Color.WHITE

                val x0 = message.cropRect.x + (message.cropRect.width / 2.0)
                val y0 = message.cropRect.y + (message.cropRect.height / 2.0)
                var angle = 0.0

                for (ray in message.raysList.take(message.raysCount)) {
                    val x1 = x0 + (ray * sin(angle))
                    val y1 = y0 + (ray * cos(angle))

                    display.strokeLine(x0, y0, x1, y1)
                    angle += message.rayInterval
                }

                display.lineWidth = 4.0
                display.stroke = Color.LIME
                val diameter = message.mirrorRadius * 2.0
                display.strokeOval(x0 - message.mirrorRadius, y0 - message.mirrorRadius, diameter, diameter)
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
            disconnect()
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
                // convert min and max from RGB on Omicam's end to whatever colour space we're in here
                // sorry for how ugly this looks, it's just an ugly problem in general I think
                // could probably make it better with generics and reflection but I can't really be stuffed
                val minColour = RGB(thresh.minList[0], thresh.minList[1], thresh.minList[2]).convertToAnyColour(colourSpace)
                val maxColour = RGB(thresh.maxList[0], thresh.maxList[1], thresh.maxList[2]).convertToAnyColour(colourSpace)
                val convertedThresh = RemoteDebug.RDThreshold.newBuilder().apply {
                    addAllMin(listOf(minColour[0], minColour[1], minColour[2]))
                    addAllMax(listOf(maxColour[0], maxColour[1], maxColour[2]))
                }.build()

                Logger.debug("Object ${FieldObjects.values()[i]} original min: ${thresh.minList}, converted min: ${convertedThresh.minList}    " +
                        "original max: ${thresh.maxList}, converted max: ${convertedThresh.maxList} (RGB -> ${minColour::class.simpleName})")
                objectThresholds[FieldObjects.values()[i]] = convertedThresh
            }

            // notify awaiting threads that the new data has been received
            synchronized(newThreshReceivedToken) { newThreshReceivedToken.notifyAll() }
        }, {
            Logger.error("Failed to update threshold values")
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
                Logger.error("No threshold for $newObj? (objThresholds[newObj] == null)")
                runLater {
                    selectBox.isDisable = false
                    Utils.showGenericAlert(Alert.AlertType.ERROR,
                        "Corrupted information was received from Omicam while trying to change objects.\n" +
                                "This could indicate a slow connection. Please restart the SBC and try again.",
                        "Error switching objects"
                    )
                }
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

                // set min then max slider values, this should already be in correct colour space as its done earlier
                for ((i, colour) in colourSpace.friendlyName.withIndex()) {
                    sliders["${colour}Min"]?.value = newObjThresh.minList[i].toDouble()
                }
                for ((i, colour) in colourSpace.friendlyName.withIndex()) {
                    sliders["${colour}Max"]?.value = newObjThresh.maxList[i].toDouble()
                }

                selectBox.isDisable = false
            }, {
                Utils.showGenericAlert(Alert.AlertType.ERROR, "Failed to update to new selected object.\n" +
                        "If this persists please restart the SBC and try again.",
                    "Communication failure")
                selectBox.isDisable = false
            })
        }
    }

    /** sends threshold slider value change to Omicontrol **/
    private fun publishSliderValueChange(type: String, value: Int, channelIdx: Int){
        // convert from current colour space to RGB
        val colour = if (type.toLowerCase() == "min") colourSpaceToColour(colourSpace, minColour)
                        else colourSpaceToColour(colourSpace, maxColour)
        val rgbColour = colour.toRGB()
        // Logger.trace("SLIDER Colour value in ${colourSpace.friendlyName}: $colour, in RGB: $rgbColour")

        val msg = RemoteDebug.DebugCommand.newBuilder().apply {
            messageId = DebugCommands.CMD_THRESHOLDS_SET.ordinal
            minMax = type.toLowerCase() == "min"
            colourChannel = channelIdx
            this.value = rgbColour.getChannel(channelIdx)
        }.build()
        CONNECTION_MANAGER.dispatchCommand(command=msg, ignoreErrors=true)
    }

    /**
     * Generates a JavaFX field containing a slider and label for the given colour channel in the threshold
     * @param colour the name of the colour channel, eg "R"
     * @param type "min" or "max"
     * @param colourIdx the position of the current colour index in the colour space
     **/
    private fun generateSlider(colour: String, type: String, colourIdx: Int): Field {
        Logger.trace("Generating slider for colour $colour, type $type, channel index: $colourIdx")
        return field("$colour $type") {
            val colourSlider = slider(min=colourSpace.minRange[colourIdx], max=colourSpace.maxRange[colourIdx]){
                blockIncrement = 1.0
                majorTickUnit = 1.0
                minorTickCount = 0
                isSnapToTicks = true
                isDisable = true // since we start out in OBJ_NONE and you can't edit it

                // when the mouse is pressed start the drag timer
                setOnMousePressed {
                    grabSendTimer = fixedRateTimer(name="GrabSendTimer", period=GRAB_SEND_TIMER_PERIOD, action = {
                        publishSliderValueChange(type, value.toInt(), colourIdx)
                    })
                }
                // when the mouse is finished being moved, cancel the grab timer and send the request to Omicam
                setOnMouseReleased {
                    grabSendTimer?.cancel()
                    grabSendTimer?.purge()
                    publishSliderValueChange(type, value.toInt(), colourIdx)
                }
                // left and right arrow keys
                setOnKeyReleased {
                    if (it.code == KeyCode.LEFT || it.code == KeyCode.RIGHT)
                        publishSliderValueChange(type, value.toInt(), colourIdx)
                }
            }
            sliders["$colour$type"] = colourSlider

            label("000"){
                colourSlider.valueProperty().addListener { _, _, newValue ->
                    text = newValue.toInt().toString().padStart(3, '0')

                    if (type.toLowerCase() == "min"){
                        minColour[colourIdx] = newValue.toInt()
                    } else {
                        maxColour[colourIdx] = newValue.toInt()
                    }
                }
            }
        }
    }

    /** Clears the current slider hashmap and vbox, and re-adds them **/
    private fun regenerateSliders(){
        Logger.trace("Regenerating sliders")
        sliderVbox.clear()
        sliders.clear()

        for ((i, channel) in colourSpace.friendlyName.withIndex()) {
            sliderVbox.add(fieldset {
                add(generateSlider(channel.toString(), "Min", i))
                add(generateSlider(channel.toString(), "Max", i))
            })
        }
    }

    private fun disconnect(){
        Logger.debug("Disconnecting...")
        CONNECTION_MANAGER.disconnect()
        EVENT_BUS.unregister(this@CameraView)
        // if the user is stupid enough to press CTRL+D _while_ dragging the slider, stop them
        grabSendTimer?.cancel()
        grabSendTimer?.purge()
        Utils.transitionMetro(this@CameraView, ConnectView())
    }

    override val root = vbox {
        setPrefSize(1600.0, 900.0)
        EVENT_BUS.register(this@CameraView)
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
            menu("Actions") {
                item("Enter sleep mode"){
                    setOnAction {
                        val msg = RemoteDebug.DebugCommand.newBuilder()
                            .setMessageId(DebugCommands.CMD_SLEEP_ENTER.ordinal)
                            .build()
                        CONNECTION_MANAGER.dispatchCommand(msg, {
                            Logger.info("Acknowledgement of sleep received, disconnecting")
                            runLater {
                                Utils.showGenericAlert(Alert.AlertType.INFORMATION,
                                    "Omicam has entered a low power sleep state.\n\nVision will not be functional until" +
                                            " you reconnect, which automatically exits sleep mode.", "Sleep mode")
                                disconnect()
                            }
                        })
                    }
                    accelerator = KeyCodeCombination(KeyCode.E, KeyCombination.CONTROL_DOWN)
                }
                item("Calibrate camera"){
                    setOnAction {
                        Logger.info("Changing to camera calibration screen")
                        EVENT_BUS.unregister(this@CameraView)
                        Utils.transitionMetro(this@CameraView, CalibrationView())
                    }
                }
                item("Save next frame to disk"){
                    setOnAction {
                        saveNextToDisk = true
                    }
                }
                item("Save config"){
                    accelerator = KeyCodeCombination(KeyCode.S, KeyCombination.CONTROL_DOWN)
                    setOnAction {
                        val msg = RemoteDebug.DebugCommand.newBuilder()
                            .setMessageId(DebugCommands.CMD_THRESHOLDS_WRITE_DISK.ordinal)
                            .build()
                        CONNECTION_MANAGER.dispatchCommand(msg, {
                            Utils.showGenericAlert(Alert.AlertType.INFORMATION,
                                "Your settings have been written to the remote Omicam config file.",
                            "Config saved successfully"
                            )
                        })
                    }
                }
                item("Reload config"){
                    accelerator = KeyCodeCombination(KeyCode.R, KeyCombination.CONTROL_DOWN)
                    setOnAction {
                        val msg = RemoteDebug.DebugCommand.newBuilder()
                            .setMessageId(DebugCommands.CMD_RELOAD_CONFIG.ordinal)
                            .build()
                        CONNECTION_MANAGER.dispatchCommand(msg, {
                            Utils.showGenericAlert(Alert.AlertType.INFORMATION, "Omicam settings have been updated from disk.",
                                "Reloaded config successfully.")
                        })
                    }
                }
            }
            menu("Settings"){
                checkmenuitem("Multi-slide mode"){
                    selectedProperty().addListener { _, _, newValue ->
                        // FIXME finish this off - or consider removing
                        multiSlide = newValue
                    }
                }
            }
            menu("Help") {
                item("About").setOnAction {
                    Utils.showGenericAlert(Alert.AlertType.INFORMATION, "Copyright (c) 2019-2020 Team Omicron. See LICENSE.txt",
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
                    sliderVbox = vbox {}
                    regenerateSliders()

                    // object selection dropdown
                    fieldset {
                        field {
                            label("Select object:")
                            selectBox = combobox {
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
                            label("Colour space:")
                            combobox<String> {
                                items = FXCollections.observableArrayList("RGB", "HSV", "HSL", "YUV", "LAB")
                                selectionModel.selectFirst()

                                valueProperty().addListener { _, _, newValue ->
                                    /*
                                    To update colour spaces, we will need to:
                                    1. set the new selected colour space enum
                                    2. convert threshold values from current colour space
                                    3. update labels
                                    */

                                    // we could probably do some reflection trickery here but it's probably not worth it
                                    when (newValue){
                                        "RGB" -> colourSpace = RGB
                                        "HSV" -> colourSpace = HSV
                                        "HSL" -> colourSpace = HSL
                                        "YUV" -> Logger.error("YUV is not yet supported")
                                        "LAB" -> colourSpace = LAB
                                        else -> Logger.error("Invalid colour space: $newValue")
                                    }
                                    regenerateSliders()

                                    // this is the best way I can think of to basically re-get the slider values, since
                                    // we essentially just destroyed them, though it requires fetching from the server
                                    // essentially, we just re-select the currently selected object
                                    selectNewObject(selectedObject.ordinal)

                                    // TODO we should also send ALL the new values to the remote for each channel to make
                                    // sure it's still valid
                                    for ((key, value) in sliders.entries){

                                    }
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
                            label("Raycast debug: ")
                            checkbox {
                                selectedProperty().addListener { _, _, newValue ->
                                    localiserDebug = newValue
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
                            temperatureLabel = label("Temperature: Unknown")
                        }
                    }

                    fieldset {
                        field {
                            lastPingLabel = label("Last ping: None recorded")
                        }
                    }

                    fieldset {
                        field {
                            bandwidthLabel = label("Packet size: None recorded")
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
            button("Switch to field view"){
                setOnAction {
                    Logger.debug("Switching to field view")
                    EVENT_BUS.unregister(this@CameraView)
                    Utils.transitionMetro(this@CameraView, FieldView())
                }
            }
        }

        @Suppress("ConstantConditionIf")
        if (DEBUG_CAMERA_VIEW){
            display.fill = Color.WHITE
            display.fillRect(0.0, 0.0, CANVAS_WIDTH, CANVAS_HEIGHT)
        }
    }
}