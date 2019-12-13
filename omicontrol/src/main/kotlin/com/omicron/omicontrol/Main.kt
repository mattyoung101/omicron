package com.omicron.omicontrol

import com.google.common.eventbus.DeadEvent
import com.google.common.eventbus.Subscribe
import com.omicron.omicontrol.views.ConnectView
import org.tinylog.kotlin.Logger
import tornadofx.App
import tornadofx.importStylesheet
import tornadofx.launch
import java.nio.file.Paths
import kotlin.system.exitProcess

class OmicontrolApp : App(ConnectView::class, Styles::class)

object Main {
    @Subscribe
    fun receiveDeadEvent(event: DeadEvent){
        Logger.warn("Warning: Failed to deliver event of class ${event.event.javaClass.simpleName} as it has no subscribers")
    }

    @JvmStatic
    fun main(args: Array<String>){
        System.setProperty("tinylog.configuration", "tinylog.properties")
        Logger.info("Omicontrol v${OMICONTROL_VERSION} - Copyright (c) 2019 Team Omicron. All rights reserved.")
        importStylesheet(Paths.get("DarkTheme.css").toUri().toURL().toExternalForm())
        launch<OmicontrolApp>(args)
        exitProcess(0)
    }
}