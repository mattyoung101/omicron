package com.omicron.omicontrol

import com.google.common.eventbus.DeadEvent
import com.google.common.eventbus.Subscribe
import com.omicron.omicontrol.views.ConnectView
import org.apache.commons.lang3.SystemUtils
import org.tinylog.kotlin.Logger
import tornadofx.App
import tornadofx.importStylesheet
import tornadofx.launch
import java.nio.file.Paths
import kotlin.system.exitProcess

class OmicontrolApp : App(ConnectView::class, Styles::class)

object Main {
    @JvmStatic
    fun main(args: Array<String>){
        System.setProperty("tinylog.configuration", "tinylog.properties")
        Logger.info("Omicontrol v${OMICONTROL_VERSION} - Copyright (c) 2019-2020 Team Omicron.")

        Logger.debug("OS name: ${SystemUtils.OS_NAME}")
        if (SystemUtils.IS_OS_MAC){
            Logger.warn("This is a Mac, scaling/DPI workaround will be enabled")
        }

        importStylesheet(Paths.get("DarkTheme.css").toUri().toURL().toExternalForm())
        launch<OmicontrolApp>(args)
        exitProcess(0)
    }
}