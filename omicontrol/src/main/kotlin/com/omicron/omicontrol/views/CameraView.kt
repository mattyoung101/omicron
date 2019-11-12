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
import javafx.scene.paint.Color
import kotlin.math.floor

class CameraView : View() {
    init {
        reloadStylesheetsOnFocus()
        title = "Camera View | Omicontrol"
        EVENT_BUS.register(this)
        println("Making new Camera View")
    }
    /** displays the normal image **/
    private lateinit var defaultImage: ImageView
    /** displays the ball thresh image **/
    private lateinit var ballThreshDisplay: GraphicsContext
    private val compressor = Inflater()

    @ExperimentalUnsignedTypes
    @Subscribe
    fun receiveMessageEvent(message: RemoteDebug.DebugFrame) {
        // decompress the threshold image
        compressor.reset()
        compressor.setInput(message.ballThreshImage.toByteArray())
        val outBuf = ByteArray(1048576) // 1 megabyte (in binary bytes)
        val bytes = compressor.inflate(outBuf)

        runLater {
            val img = Image(ByteArrayInputStream(message.defaultImage.toByteArray()))
            defaultImage.image = img

            // because the threshold image is 1 bit, we have to clone the colour channels to make a valid RGB image
            ballThreshDisplay.clearRect(0.0, 0.0, IMAGE_WIDTH, IMAGE_HEIGHT)
            for (i in 0 until bytes) {
                val byte: Int = outBuf[i].toUByte().toInt()
                if (byte == 0) continue; // skip black pixels
                val x = i % IMAGE_WIDTH.toInt()
                val y = floor(i / IMAGE_WIDTH).toInt()
                ballThreshDisplay.pixelWriter.setColor(x, y, Color.ORANGE)
            }
        }
    }

    @Subscribe
    fun receiveRemoteShutdownEvent(message: RemoteShutdownEvent){
        println("Received remote shutdown event")
        runLater {
            Utils.showGenericAlert(Alert.AlertType.ERROR, "The remote connection has unexpectedly terminated.\n" +
                    "Please check Omicam is still running and try again.\n\nError description: Protobuf message was null",
                "Connection error")
            Utils.transitionMetro(this, ConnectView())
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
                item("Reboot remote").setOnAction {
                    // send reboot command id
                }
                item("Shutdown remote").setOnAction {
                    // send shutdown command id
                }
                item("Save thresholds").setOnAction {
                    // send save thresholds command id
                }
            }
            menu("Help") {
                item("What's this?").setOnAction {
                    Utils.showGenericAlert(Alert.AlertType.INFORMATION, """
                        In the camera view, you can see the output of the camera and tune new thresholds. Use the pane
                        on the side to select different colours and view their threshold masks. Use the sliders below the 
                        select box to tune the thresholds. Your results will be reflected in near real-time.
                        
                        Use the Actions menu to run commands such as shutdown and rebooting the Pi. It's VERY IMPORTANT
                        that you select Actions > Save Thresholds before changing colours, in order to save your tuned
                        thresholds to disk.
                        
                        This view only lets you interact with the camera. To interact with the robot, please visit
                        the main menu and select "Robot" instead of "Camera".
                    """.trimIndent(), "Camera View Help")
                }
                item("About").setOnAction {
                    Utils.showGenericAlert(Alert.AlertType.INFORMATION, "Copyright (c) 2019 Team Omicron. See LICENSE.txt",
                        "Omicontrol v${VERSION}", "About")
                }
            }
        }

        vbox {
            hbox {
                // we render the image and the thresh image on top of each other. the thresh image is rendered
                // with add blending (so white pixels black pixels are see through and white pixels are white)
                // TODO in future we need to add a toggle pane to select the different channels of the image

                stackpane {
                    defaultImage = imageview().apply {
//                        blendMode = BlendMode.OVERLAY
                    }
                    canvas(IMAGE_WIDTH, IMAGE_HEIGHT) { ballThreshDisplay = graphicsContext2D }.apply {
//                        blendMode = BlendMode.OVERLAY
                    }
                }
                alignment = Pos.CENTER_RIGHT
            }
            alignment = Pos.CENTER_RIGHT
        }

        vbox {
            paddingTop = 25.0
            alignment = Pos.CENTER
            button("Switch to robot view")
        }
    }
}