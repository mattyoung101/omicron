package com.omicron.omicontrol

import RemoteDebug
import java.net.InetSocketAddress
import java.net.Socket
import java.nio.ByteBuffer
import kotlin.concurrent.thread

class ConnectionManager {
    private val socket = Socket()
    private val msgBuffer = ByteBuffer.allocateDirect(512000) // 512 KB, current buffers are around 55 KB
    private var requestShutdown = false

    private val tcpThread = thread(start=false, name="TCPThread"){
        val stream = socket.getInputStream()
        while (true){
            if (Thread.interrupted() || requestShutdown){
                println("TCP thread interrupted, closing socket")
                socket.close()
                return@thread
            }

            val msg = RemoteDebug.DebugFrame.parseDelimitedFrom(stream)
            println("received message???? default image: ${msg.defaultImage.size()}")
            Values.eventBus.post(msg)
        }
    }

    init {
        Runtime.getRuntime().addShutdownHook(thread(start=false){
            println("Shutdown hook running for ConnectionManager")
            disconnect()
        })
    }

    fun connect(ip: String = Values.REMOTE_IP, port: Int = Values.REMOTE_PORT){
        println("Connecting to remote at $ip:$port")
        socket.connect(InetSocketAddress(
            Values.REMOTE_IP,
            Values.REMOTE_PORT
        ))
        tcpThread.start()
        println("Connected successfully")
    }

    fun disconnect(){
        requestShutdown = true
    }
}