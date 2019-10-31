package com.omicron.omicontrol

import com.google.common.eventbus.EventBus

object Values {
    const val REMOTE_IP = "192.168.1.6"
    const val REMOTE_PORT = 42708
    val connectionManager = ConnectionManager()
    val eventBus = EventBus()
}