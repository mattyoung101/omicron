package com.omicron.omicontrol

import RemoteDebug
import java.net.InetSocketAddress
import java.net.Socket
import java.nio.ByteBuffer
import kotlin.concurrent.thread

class ConnectionManager {
    private var socket = Socket()
    private var requestShutdown = false
    private var tcpThread = thread(start=false){}

    /** code run by tcpThread **/
    private fun tcpThreadFun(){
        val stream = socket.getInputStream()
        println("TCP thread started")

        while (true){
            if (Thread.interrupted() || requestShutdown){
                println("TCP thread interrupted, closing socket")
                socket.close()
                requestShutdown = false
                return
            }

            val msg = RemoteDebug.DebugFrame.parseDelimitedFrom(stream)
            if (msg == null){
                println("Remote has likely disconnected")
                EVENT_BUS.post(RemoteShutdownEvent())
                Thread.currentThread().interrupt()
            } else {
                EVENT_BUS.post(msg)
            }
        }
    }

    init {
        Runtime.getRuntime().addShutdownHook(thread(start=false){
            println("Shutdown hook running for ConnectionManager")
            disconnect()
        })
    }

    fun connect(ip: String = REMOTE_IP, port: Int = REMOTE_PORT){
        println("Connecting to remote at $ip:$port")

        // re-create thread and socket (to support reconnecting)
        requestShutdown = false
        socket.close()
        socket = Socket()
        socket.connect(InetSocketAddress(REMOTE_IP, REMOTE_PORT))
        tcpThread = thread(name="TCPThread"){ tcpThreadFun() }
        println("Connected successfully")
    }

    fun disconnect(){
        requestShutdown = true
    }
}