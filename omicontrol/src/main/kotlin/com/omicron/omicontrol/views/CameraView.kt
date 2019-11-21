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
import java.awt.image.BufferedImage
import java.io.File
import java.io.FileWriter
import java.util.concurrent.atomic.AtomicBoolean
import javax.imageio.ImageIO
import kotlin.concurrent.thread
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
    private val writeToDisk = AtomicBoolean(false)
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
                ballThreshDisplay.clearRect(0.0, 0.0, IMAGE_WIDTH, IMAGE_HEIGHT)
                for (i in 0 until bytes) {
                    val byte = outBuf[i].toUByte().toInt()
                    if (byte == 0) continue // skip black pixels

                    val x = i % IMAGE_WIDTH.toInt()
                    val y = floor(i / IMAGE_WIDTH).toInt()
                    ballThreshDisplay.pixelWriter.setColor(x, y, Color.ORANGE)
                }
            }
        }

        if (writeToDisk.compareAndExchange(true, false)){
            println("Writing buffer to disk")
            run {
                val outFile = File("threshold.raw")
                val writer = outFile.printWriter()

                for ((count, i) in (0 until bytes).withIndex()) {
                    val byte: Int = outBuf[i].toUByte().toInt()
                    writer.print(if (byte == 0) 0 else 1)

                    if (count % IMAGE_WIDTH.toInt() == 0) {
                        writer.print("\n")
                    }
                }
                writer.close()
            }

            run {
                val outFile = File("threshold.png")
                val img = BufferedImage(IMAGE_WIDTH.toInt(), IMAGE_HEIGHT.toInt(), BufferedImage.TYPE_BYTE_GRAY)
                for (i in 0 until bytes) {
                    val x = i % IMAGE_WIDTH.toInt()
                    val y = floor(i / IMAGE_WIDTH).toInt()
                    val byte: Int = outBuf[i].toUByte().toInt()
                    img.setRGB(x, y, if (byte == 0) java.awt.Color.BLACK.rgb else java.awt.Color.WHITE.rgb)
                }
                ImageIO.write(img, "png", outFile)
            }
            println("Written successfully")
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
                item("Save thresholds").setOnAction {
                    // send save thresholds command id
                }
                item("Write next threshold buffer to disk").setOnAction {
                    writeToDisk.set(true)
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

        hbox {
            vbox {
                label("For fuck sake")
                alignment = Pos.TOP_LEFT
            }

            vbox {
                stackpane {
                    defaultImage = imageview()
                    canvas(IMAGE_WIDTH, IMAGE_HEIGHT) { ballThreshDisplay = graphicsContext2D }
                }
                alignment = Pos.TOP_RIGHT
            }

            alignment = Pos.TOP_LEFT
        }

        vbox {
            paddingTop = 25.0
            alignment = Pos.CENTER
            button("Switch to robot view")
        }
    }
}