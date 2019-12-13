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
import javafx.scene.canvas.Canvas
import javafx.scene.control.Label
import javafx.scene.control.Slider
import javafx.scene.layout.Priority
import javafx.scene.paint.Color
import org.tinylog.kotlin.Logger
import kotlin.math.floor

class CameraView : View() {
    /** displays the normal image **/
    private lateinit var defaultImage: ImageView
    /** displays the thresh image(s) **/
    private lateinit var threshDisplay: GraphicsContext
    private lateinit var threshDisplayScaled: GraphicsContext
    private lateinit var temperatureLabel: Label
    private val compressor = Inflater()
    private var renderThresholds = true
    private var hideCameraFrame = false
    /** mapping between each field object and its threshold **/
    private val objectThresholds = hashMapOf<FieldObjects, RemoteDebug.RDThreshold>()

    init {
        reloadStylesheetsOnFocus()
        title = "Camera View | Omicontrol"
        EVENT_BUS.register(this)

        // get threshold slider values
        val command = RemoteDebug.DebugCommand
            .newBuilder()
            .setMessageId(DebugCommands.CMD_THRESHOLDS_GET_ALL.ordinal)
            .build()
        CONNECTION_MANAGER.encodeAndSend(command, {
            for ((i, thresh) in it.allThresholdsList.withIndex()){
                println("Object ${FieldObjects.values()[i]} min: ${thresh.minList}, max: ${thresh.maxList}")
                objectThresholds[FieldObjects.values()[i]] = thresh
            }
        })
    }

    @ExperimentalUnsignedTypes
    @Subscribe
    fun receiveMessageEvent(message: RemoteDebug.DebugFrame) {
        // decompress the threshold image
        compressor.reset()
        compressor.setInput(message.ballThreshImage.toByteArray())
        val outBuf = ByteArray(1024 * 1024) // 1 megabyte (in binary bytes)
        val bytes = compressor.inflate(outBuf)

        runLater {
            val begin = System.currentTimeMillis()
            val temp = message.temperature
            temperatureLabel.text = "${String.format("%.2f", temp)}°C"
            if (temp >= 75.0){
                temperatureLabel.textFill = Color.RED
            } else {
                temperatureLabel.textFill = Color.WHITE
            }

            val img = Image(ByteArrayInputStream(message.defaultImage.toByteArray()))

            // write the received image data to the canvas, skipping black pixels (this fakes blend mode which won't work
            // for some reason)
            if (renderThresholds) {
                threshDisplay.clearRect(0.0, 0.0, IMAGE_WIDTH, IMAGE_HEIGHT)
                if (!hideCameraFrame) threshDisplay.drawImage(img, 0.0, 0.0, IMAGE_WIDTH, IMAGE_HEIGHT)

                // FIXME this is very slow in the order of 10-30 ms, find some way to speed it up
                for (i in 0 until bytes) {
                    val byte = outBuf[i].toUByte().toInt()
                    if (byte == 0) continue

                    val x = i % IMAGE_WIDTH.toInt()
                    val y = floor(i / IMAGE_WIDTH).toInt()
                    threshDisplay.pixelWriter.setColor(x, y, Color.ORANGE)
                }

                // draw ball centre
                threshDisplay.fill = Color.RED
                val ballX = message.ballCentroid.x.toDouble()
                val ballY = message.ballCentroid.y.toDouble()
                threshDisplay.fillOval(ballX, ballY, 10.0, 10.0)

                // draw ball bounding box
                threshDisplay.stroke = Color.RED
                threshDisplay.lineWidth = 4.0
                threshDisplay.strokeRect(message.ballRect.x.toDouble(), message.ballRect.y.toDouble(),
                    message.ballRect.width.toDouble(), message.ballRect.height.toDouble())
            }

            // TODO this method is apparently incredibly inefficient, so I reckon we remove it and go back to what we had before
            val fbo = threshDisplay.canvas.snapshot(null, null)
            threshDisplayScaled.clearRect(0.0, 0.0, IMAGE_WIDTH, IMAGE_HEIGHT)
            threshDisplayScaled.drawImage(fbo, 0.0, 0.0, IMAGE_WIDTH * IMAGE_SIZE_SCALAR, IMAGE_HEIGHT * IMAGE_SIZE_SCALAR)

            // Logger.trace("Frame update took: ${System.currentTimeMillis() - begin} ms")
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

    private fun applyColourSliderProperties(slider: Slider){
        slider.apply {
            blockIncrement = 1.0
            majorTickUnit = 1.0
            minorTickCount = 0
            isSnapToTicks = true
        }
    }

    private fun generateSlider(colour: String, type: String): Field {
        return field("$colour $type") {
            val colourSlider = slider(min=0, max=255){
                applyColourSliderProperties(this)
            }

            label("000"){
                colourSlider.valueProperty().addListener { _, _, newValue ->
                    text = newValue.toInt().toString().padStart(3, '0')
                }
                cursor = Cursor.HAND

                // allow the user to manually input the threshold value
                setOnMouseClicked {
                    val result = Utils.showTextInputDialog("Enter new value (0-255):", "Manual threshold input",
                        default = colourSlider.value.toInt().toString())
                    val newValue = result.toIntOrNull() ?: return@setOnMouseClicked

                    if (newValue > 255 || newValue < 0){
                        Utils.showGenericAlert(Alert.AlertType.ERROR, "\"$result\" is out of range (required range: 0-255).",
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
                item("Reboot camera").setOnAction {
                    // send reboot command id
                }
                item("Shutdown camera").setOnAction {
                    // send shutdown command id
                }
                item("Save config"){
                    accelerator = KeyCodeCombination(KeyCode.S, KeyCombination.CONTROL_DOWN)
                    setOnAction {
                        val msg = RemoteDebug.DebugCommand.newBuilder()
                            .setMessageId(DebugCommands.CMD_THRESHOLDS_WRITE_DISK.ordinal)
                            .build()
                        CONNECTION_MANAGER.encodeAndSend(msg, {
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

                    // hidden canvas which is actually rendered to of original size
                    Canvas(IMAGE_WIDTH, IMAGE_HEIGHT).apply {
                        threshDisplay = graphicsContext2D
                    }

                    // real canvas which the frame buffer is copied to and downscaled
                    canvas(IMAGE_WIDTH * IMAGE_SIZE_SCALAR, IMAGE_HEIGHT * IMAGE_SIZE_SCALAR){
                        threshDisplayScaled = graphicsContext2D
                    }
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
            threshDisplayScaled.fill = Color.WHITE
            threshDisplayScaled.fillRect(0.0, 0.0, IMAGE_WIDTH, IMAGE_HEIGHT)
        }
    }
}