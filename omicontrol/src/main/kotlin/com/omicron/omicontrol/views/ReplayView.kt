package com.omicron.omicontrol.views

import com.omicron.omicontrol.*
import com.omicron.omicontrol.field.Robot
import javafx.geometry.Point2D
import javafx.geometry.Pos
import javafx.scene.canvas.GraphicsContext
import javafx.scene.control.Alert
import javafx.scene.control.Label
import javafx.scene.input.KeyCode
import javafx.scene.input.KeyCodeCombination
import javafx.scene.input.KeyCombination
import javafx.scene.layout.Priority
import org.greenrobot.eventbus.Subscribe
import org.tinylog.kotlin.Logger
import tornadofx.*
import kotlin.system.exitProcess

/**
 * NOTE: it's not guaranteed that we'll use this instead of the FieldView, it probably makes more sense to use field view
 */
class ReplayView : View() {
    private lateinit var display: GraphicsContext
    private var mousePos = Point2D(0.0, 0.0)
    private lateinit var ballLabel: Label
    private lateinit var statusLabel: Label
    private val robots = listOf(
        Robot(0),
        Robot(1)
    )

    /** Stop stupid event bus exceptions */
    @Subscribe
    fun shutUp(nothing: String){}

    override val root = vbox {
        setPrefSize(1600.0, 1000.0)
        EVENT_BUS.register(this@ReplayView)

        menubar {
            menu("File") {
                item("Quit to menu") {
                    setOnAction {
                        EVENT_BUS.unregister(this@ReplayView)
                        Utils.transitionMetro(this@ReplayView, ConnectView())
                    }
                }
                item("Exit Omicontrol") {
                    setOnAction {
                        exitProcess(0)
                    }
                    accelerator = KeyCodeCombination(KeyCode.Q, KeyCombination.CONTROL_DOWN)
                }
            }
            menu("Help") {
                item("About").setOnAction {
                    Utils.showGenericAlert(
                        Alert.AlertType.INFORMATION, "Copyright (c) 2019-2020 Team Omicron. See LICENSE.txt",
                        "Omicontrol v$OMICONTROL_VERSION", "About"
                    )
                }
            }
        }

        hbox {
            vbox {
                canvas(FIELD_CANVAS_WIDTH, FIELD_CANVAS_HEIGHT) {
                    display = graphicsContext2D

                    setOnMouseMoved {
                        mousePos = Point2D(it.x, it.y)
                    }

                    setOnMouseClicked {
                        val fieldCanvasPos = Point2D(it.x, it.y)
                        val fieldRealPos = fieldCanvasPos.toRealPosition()
                        Logger.info("Mouse clicked at real: $fieldRealPos, canvas: $fieldCanvasPos")
                    }
                }
                alignment = Pos.TOP_RIGHT
            }

            vbox {
                hbox {
                    label("Replay Viewer") {
                        addClass(Styles.bigLabel)
                        alignment = Pos.TOP_CENTER
                    }
                    hgrow = Priority.ALWAYS
                    alignment = Pos.TOP_CENTER
                }

                form {
                    fieldset {
                        field {
                            label("Replay status:") { addClass(Styles.boldLabel) }
                            statusLabel = label("Unknown")
                        }
                    }
                    fieldset {
                        field {
                            label("Robot 0:") { addClass(Styles.boldLabel) }
                        }
                        field {
                            robots[0].positionLabel = label("Position: Unknown")
                        }
                        field {
                            label("FSM state: Unknown")
                        }
                    }
                    fieldset {
                        field {
                            label("Robot 1:") { addClass(Styles.boldLabel) }
                        }
                        field {
                            robots[1].positionLabel = label("Position: Unknown")
                        }
                        field {
                            label("FSM state: Unknown")
                        }
                    }

                    fieldset {
                        field {
                            label("Ball:") { addClass(Styles.boldLabel) }
                            ballLabel = label("Unknown")
                        }
                    }

                    fieldset {
                        field {
                            label("Actions:") { addClass(Styles.boldLabel) }
                        }

                        field {
                            button("Resume")
                            button("Pause")
                            button("Restart")
                        }
                    }

                    hgrow = Priority.ALWAYS
                    alignment = Pos.TOP_LEFT
                }

                hgrow = Priority.ALWAYS
            }
        }
    }
}