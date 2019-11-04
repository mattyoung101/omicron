package com.omicron.omicontrol

import com.google.common.eventbus.DeadEvent
import com.google.common.eventbus.Subscribe
import com.omicron.omicontrol.views.CameraView
import com.omicron.omicontrol.views.ConnectView
import tornadofx.App
import tornadofx.importStylesheet
import tornadofx.launch
import java.nio.file.Paths
import kotlin.system.exitProcess

class OmicontrolApp : App(ConnectView::class, Styles::class)

object Main {
    @Subscribe
    fun receiveDeadEvent(event: DeadEvent){
        println("Warning: Failed to deliver event of class ${event.event.javaClass.simpleName} as it has no subscribers")
    }

    @JvmStatic
    fun main(args: Array<String>){
        importStylesheet(Paths.get("DarkTheme.css").toUri().toURL().toExternalForm())
        launch<OmicontrolApp>(args)
        exitProcess(0)
    }
}