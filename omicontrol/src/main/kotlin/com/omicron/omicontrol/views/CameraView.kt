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
import java.util.concurrent.atomic.AtomicBoolean
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
    /** displays the thresh image(s) **/
    private lateinit var threshDisplay: GraphicsContext
    private val compressor = Inflater()
    private var renderThresholds = true

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

            // write the received image data to the canvas, skipping black pixels (this fakes blend mode which won't work
            // for some reason)
            if (renderThresholds) {
                threshDisplay.clearRect(0.0, 0.0, IMAGE_WIDTH, IMAGE_HEIGHT)

                // FIXME this is very slow in the order of 10-30 ms, find some way to speed it up
                for (i in 0 until bytes) {
                    val byte = outBuf[i].toUByte().toInt()
                    if (byte == 0) continue // skip black pixels

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
                item("Reboot camera").setOnAction {
                    // send reboot command id
                }
                item("Shutdown camera").setOnAction {
                    // send shutdown command id
                }
                item("Save config").setOnAction {
                    // send save thresholds command id
                }
            }
            menu("Help") {
                item("What's this?").setOnAction {
                    Utils.showGenericAlert(Alert.AlertType.INFORMATION, """
                        In the camera view, you can see the output of the camera and tune new thresholds. Use the pane
                        on the side to select different colours and view their threshold masks. Use the sliders below the 
                        select box to tune the thresholds. Your results will be reflected in near real-time.
                        
                        Use the Actions menu to run commands such as shutdown and rebooting the Jetson. It's VERY IMPORTANT
                        that you select Actions > Save Config before changing colours, in order to save your tuned
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

        hbox {
            vbox {
                alignment = Pos.TOP_CENTER
                minWidth = 240.0
                label("Information")

                hbox {
                    label("R Min")
                    slider(min = 0, max = 255)
                }
                hbox {
                    label("R Max")
                    slider(min = 0, max = 255)
                }

                hbox {
                    label("R Min")
                    slider(min = 0, max = 255)
                }
                hbox {
                    label("R Max")
                    slider(min = 0, max = 255)
                }

                hbox {
                    label("B Min")
                    slider(min = 0, max = 255)
                }
                hbox {
                    label("B Max")
                    slider(min = 0, max = 255)
                }

                hbox {
                    label("G Min")
                    slider(min = 0, max = 255)
                }
                hbox {
                    label("G Max")
                    slider(min = 0, max = 255)
                }
            }

            vbox {
                stackpane {
                    defaultImage = imageview()
                    canvas(IMAGE_WIDTH, IMAGE_HEIGHT) { threshDisplay = graphicsContext2D }
                }
                alignment = Pos.TOP_RIGHT
            }

            alignment = Pos.TOP_RIGHT
        }

        vbox {
            paddingTop = 25.0
            alignment = Pos.CENTER
            button("Switch to robot view")
        }
    }
}