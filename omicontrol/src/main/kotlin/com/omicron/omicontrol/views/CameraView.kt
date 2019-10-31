package com.omicron.omicontrol.views

import com.google.common.eventbus.Subscribe
import com.omicron.omicontrol.Values
import javafx.geometry.Pos
import javafx.scene.Parent
import javafx.scene.image.Image
import javafx.scene.image.ImageView
import tornadofx.View
import tornadofx.imageview
import tornadofx.reloadStylesheetsOnFocus
import tornadofx.vbox
import java.io.ByteArrayInputStream

class CameraView: View() {
    init {
        reloadStylesheetsOnFocus()
        title = "Connect to Robot | Omicontrol"
        Values.eventBus.register(this)
    }
    private lateinit var imageView: ImageView

    @Subscribe
    fun receiveMessageEvent(message: RemoteDebug.DebugFrame){
//        println("Received message event with size: ${message.defaultImage.size()}")
        imageView.image = Image(ByteArrayInputStream(message.defaultImage.toByteArray()))
    }

    override val root = vbox {
        setPrefSize(1600.0, 900.0)
        alignment = Pos.CENTER
        imageView = imageview()
    }
}