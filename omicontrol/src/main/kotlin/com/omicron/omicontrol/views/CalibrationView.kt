package com.omicron.omicontrol.views

import RemoteDebug
import com.omicron.omicontrol.*
import javafx.geometry.Point2D
import javafx.geometry.Pos
import javafx.scene.canvas.GraphicsContext
import javafx.scene.control.Alert
import javafx.scene.image.Image
import javafx.scene.input.KeyCode
import javafx.scene.input.KeyCodeCombination
import javafx.scene.input.KeyCombination
import javafx.scene.layout.Priority
import javafx.scene.paint.Color
import org.greenrobot.eventbus.Subscribe
import org.tinylog.kotlin.Logger
import tornadofx.*
import java.io.ByteArrayInputStream
import kotlin.system.exitProcess

class DewarpPoint(var pixelDistance: Double){
    var realDistance: Double = 0.0
}

/**
 * This is essentially a stripped down version of the ConnectView which can be used to make calibrating the
 * camera dewarp model easier.
 */
class CalibrationView : View() {
    private lateinit var display: GraphicsContext
    private val points = mutableListOf<DewarpPoint>().observable()
    /** begin point of line or null if no line being drawn **/
    private var origin: Point2D? = null
    /** end point of line or null if no line being drawn **/
    private var end: Point2D? = null
    /** whether or not the user is currently drawing a line **/
    private var selecting = false
    private var mousePos = Point2D(0.0, 0.0)
    private var latestImage: Image? = null
    private var cropRect: RemoteDebug.RDRect? = null

    init {
        reloadStylesheetsOnFocus()
        title = "Calibration View | Omicontrol"
    }

    private fun updateDisplay(){
        display.fill = Color.BLACK
        display.lineWidth = 4.0
        display.stroke = Color.RED
        display.fillRect(0.0, 0.0, CANVAS_WIDTH, CANVAS_HEIGHT)
        if (latestImage != null && cropRect != null) {
            // since Kotlin's automatic null cast checking is useless as fuck, we have to null assert these all manually
            display.drawImage(latestImage!!, cropRect!!.x.toDouble(), cropRect!!.y.toDouble(), latestImage!!.width, latestImage!!.height)
        }

        // draw the line between the two points
        if (origin != null && end != null){
            display.strokeLine(origin!!.x, origin!!.y, end!!.x, end!!.y)
        } else if (selecting){
            display.strokeLine(origin!!.x, origin!!.y, mousePos.x, mousePos.y)
        }

        // draw the origin point if we hav eit
        if (origin != null){
            display.fill = Color.RED
            display.fillOval(origin!!.x, origin!!.y, 10.0, 10.0)
        }

        // if we got no points, don't bother drawing anything
        if (!selecting && end == null) return
        val endPoint = if (selecting) mousePos else end!!

        display.fill = Color.RED
        display.fillOval(endPoint.x, endPoint.y, 10.0, 10.0)

        display.fill = Color.WHITE
        display.fillText(String.format("%.2f pixels", origin!!.distance(endPoint)), (origin!!.x + endPoint.x) / 2.0,
            (origin!!.y + endPoint.y) / 2.0)
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
            Utils.showGenericAlert(
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

    override val root = vbox {
        setPrefSize(1600.0, 900.0)
        EVENT_BUS.register(this@CalibrationView)

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
                            selecting = true
                            origin = Point2D(it.x, it.y)
                            end = null
                        } else {
                            selecting = false
                            end = Point2D(it.x, it.y)
                            points.add(DewarpPoint(origin!!.distance(end)))
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
                            tableview(points){
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
                                    points.clear()
                                    selecting = false
                                    origin = null
                                    end = null
                                }
                            }
                        }
                    }

                    fieldset {
                        field {
                            button("Calculate model"){
                                setOnAction {
                                    // TODO
                                }
                            }
                        }
                    }

                    fieldset {
                        field {
                            label("Model status: Not calculated")
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
            button("Switch to camera view"){

                setOnAction {
                    Logger.debug("Changing views")
                    EVENT_BUS.unregister(this@CalibrationView)
                    Utils.transitionMetro(this@CalibrationView, CameraView())
                }
            }
        }
    }
}
