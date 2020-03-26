package com.omicron.omicontrol

import com.github.ajalt.colormath.LAB
import com.omicron.omicontrol.views.ConnectView
import org.apache.commons.lang3.SystemUtils
import org.greenrobot.eventbus.EventBus
import org.greenrobot.eventbus.NoSubscriberEvent
import org.greenrobot.eventbus.Subscribe
import org.tinylog.kotlin.Logger
import tornadofx.App
import tornadofx.importStylesheet
import tornadofx.launch
import java.nio.file.Paths
import kotlin.system.exitProcess

class OmicontrolApp : App(ConnectView::class, Styles::class)

object Main {
    @Subscribe
    fun shutUp(event: NoSubscriberEvent){

    }

    @JvmStatic
    fun main(args: Array<String>){
        System.setProperty("tinylog.configuration", "tinylog.properties")
        Logger.info("Omicontrol v${OMICONTROL_VERSION} - Copyright (c) 2019-2020 Team Omicron.")
        EventBus.getDefault().register(this)

        // FIXME: if on windows set home to be %userprofile% as workaround

        Logger.debug("OS name: ${SystemUtils.OS_NAME}")
        if (SystemUtils.IS_OS_MAC){
            Logger.warn("This is a Mac, scaling/DPI workaround will be enabled")
        }
        if (IS_DEBUG_MODE) {
            Logger.warn("========== OMICAM DEVELOPMENT MODE IS ENABLED (make sure to turn off) ==========")
        }

        importStylesheet(Paths.get("DarkTheme.css").toUri().toURL().toExternalForm())
        launch<OmicontrolApp>(args)
        if (IS_DEBUG_MODE) {
            Logger.warn("========== Please make sure to turn off debug mode when done developing ==========")
        }
        exitProcess(0)
    }
}