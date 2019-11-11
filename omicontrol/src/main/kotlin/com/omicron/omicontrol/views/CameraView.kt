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
import javafx.scene.effect.BlendMode
import javafx.scene.paint.Color

class CameraView : View() {
    init {
        reloadStylesheetsOnFocus()
        title = "Camera View | Omicontrol"
        EVENT_BUS.register(this)
        println("Making new Camera View")
    }
    private lateinit var defaultImageView: ImageView
    private lateinit var display: GraphicsContext
    private val compressor = Inflater()

    @ExperimentalUnsignedTypes
    @Subscribe
    fun receiveMessageEvent(message: RemoteDebug.DebugFrame) {
        val img = Image(ByteArrayInputStream(message.defaultImage.toByteArray()))

        // decompress the threshold image
        compressor.reset()
        compressor.setInput(message.ballThreshImage.toByteArray())
        val outBuf = ByteArray(1048576) // 1 megabyte (in binary bytes)
        val bytes = compressor.inflate(outBuf)

        runLater {
            display.globalBlendMode = BlendMode.SRC_OVER
            display.clearRect(0.0, 0.0, IMAGE_WIDTH, IMAGE_HEIGHT)


            display.drawImage(img, 0.0, 0.0)

            // because the threshold image is 1 bit, we have to clone the colour channels to make a valid RGB image
            display.globalBlendMode = BlendMode.SCREEN
            for (i in 0 until bytes) {
                val byte: Int = outBuf[i].toUByte().toInt()
                val x = i % IMAGE_WIDTH.toInt()
                val y = i / IMAGE_WIDTH.toInt()
                display.pixelWriter.setColor(x, y, Color.grayRgb(byte))
            }
//            display.fill = Color.BLACK
//            display.fillRect(0.0, 0.0, 1280.0, 720.0)
        }
    }

    @Subscribe
    fun receiveRemoteShutdownEvent(message: RemoteShutdownEvent){
        println("Received remote shutdown event")
        runLater {
            Utils.showGenericAlert(Alert.AlertType.ERROR, "The remote connection has unexpectedly terminated.\n\n" +
                    "Please check Omicam is still running and try again.", "Connection error")
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
                item("Reboot remote")
                item("Shutdown remote")
                item("Save thresholds")
            }
            menu("Help") {
                item("About").setOnAction {
                    Utils.showGenericAlert(Alert.AlertType.INFORMATION, "Copyright (c) 2019 Team Omicron. See LICENSE.txt" +
                            "\n\nAuthors:\n- Matt Young (matt.young.1@outlook.com)",
                        "Omicontrol v${VERSION}", "About")
                }
            }
        }

        vbox {
            hbox {
                // we render the image and the thresh image on top of each other. the thresh image is rendered
                // with add blending (so white pixels black pixels are see through and white pixels are white)
                // TODO in future we need to add a toggle pane to select the different channels of the image
                canvas(IMAGE_WIDTH, IMAGE_HEIGHT){
                        display = graphicsContext2D
                }
                alignment = Pos.CENTER
            }
            alignment = Pos.CENTER
        }
    }
}