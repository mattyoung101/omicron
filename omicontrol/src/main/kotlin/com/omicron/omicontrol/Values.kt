package com.omicron.omicontrol

import com.google.common.eventbus.EventBus

const val REMOTE_IP = "127.0.0.1"
const val REMOTE_PORT = 42708
const val VERSION = "0.1"
val CONNECTION_MANAGER = ConnectionManager()
val EVENT_BUS = EventBus()
const val IMAGE_SIZE_SCALAR = 0.9
const val IMAGE_WIDTH = 1280.0 * IMAGE_SIZE_SCALAR
const val IMAGE_HEIGHT = 720.0 * IMAGE_SIZE_SCALAR
const val DEBUG_CAMERA_VIEW = true