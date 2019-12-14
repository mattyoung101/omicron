package com.omicron.omicontrol

import RemoteDebug
import org.tinylog.kotlin.Logger
import tornadofx.runLater
import java.net.InetSocketAddress
import java.net.Socket
import java.net.SocketException
import kotlin.concurrent.thread

class ConnectionManager {
    private var socket = Socket()
    private var requestShutdown = false
    private var tcpThread = thread(start=false){}
    /** threads can wait() on this to be notified when a debug command is likely to have been received **/
    private var cmdReceivedToken = Object()
    /** the last command received **/
    private var receivedCmd: RemoteDebug.DebugCommand? = null

    // code run by tcpThread, separated so it can be restarted
    private fun tcpThreadFun(){
        val sockInputStream = socket.getInputStream()
        Logger.debug("TCP thread started")

        while (true){
            if (Thread.interrupted() || requestShutdown){
                Logger.debug("TCP thread interrupted, closing socket")
                socket.close()
                requestShutdown = false
                return
            }

            var caughtException = false
            var msg: RemoteDebug.RDMsgFrame? = null

            try {
                msg = RemoteDebug.RDMsgFrame.parseDelimitedFrom(sockInputStream)
            } catch (e: SocketException){
                Logger.warn("Caught exception decoding Protobuf message: $e")
                e.printStackTrace()
                caughtException = true
            }

            if (caughtException || msg == null){
                Logger.warn("Remote has most likely disconnected (exception or null message)")
                EVENT_BUS.post(RemoteShutdownEvent())
                Thread.currentThread().interrupt()
            } else {
                when (msg.whichMessage) {
                    1 -> {
                        EVENT_BUS.post(msg.frame!!)
                    }
                    2 -> {
                        receivedCmd = msg.command
                        synchronized (cmdReceivedToken) { cmdReceivedToken.notifyAll() }
                    }
                    else -> {
                        Logger.error("Unknown wrapper message id: ${msg.whichMessage}")
                    }
                }
            }
        }
    }

    init {
        Runtime.getRuntime().addShutdownHook(thread(start=false){
            Logger.trace("Shutdown hook running for ConnectionManager")
            disconnect()
        })
    }

    fun connect(ip: String = REMOTE_IP, port: Int = REMOTE_PORT){
        Logger.info("Connecting to remote at $ip:$port")

        // re-create thread and socket (to support reconnecting)
        requestShutdown = false
        socket.close()
        socket = Socket()
        socket.connect(InetSocketAddress(REMOTE_IP, REMOTE_PORT))
        tcpThread = thread(name="TCPThread"){ tcpThreadFun() }
        Logger.info("Connected successfully")
    }

    /**
     * Encode a DebugCommand Protocol Buffer and dispatch it to Omicam
     * @param onSuccess callback to run if command was successfully received, executed in JavaFX application thread
     * @param onError callback to run if an error occurred, optional, executed in JavaFX application thread
     */
    fun dispatchCommand(command: RemoteDebug.DebugCommand, onSuccess: (RemoteDebug.DebugCommand) -> Unit = {}, onError: (String) -> Unit = {}){
        thread {
            Logger.trace("Dispatching command to Omicam")
            val outStream = socket.getOutputStream()
            command.writeDelimitedTo(outStream)
            outStream.flush()
            Logger.trace("Dispatched, awaiting response")

            synchronized(cmdReceivedToken) { cmdReceivedToken.wait(5000) }

            if (receivedCmd == null){
                Logger.warn("Received null/invalid response from Omicontrol")
                runLater { onError("Received null/invalid response from Omicontrol") }
            } else {
                Logger.trace("Received OK response from Omicontrol!")
                // yeah this is a stupid hack but for some reason there's no fuckin clone function
                val copy = RemoteDebug.DebugCommand.parseFrom(receivedCmd!!.toByteArray())
                receivedCmd = null
                runLater { onSuccess(copy) }
            }
        }
    }

    fun disconnect(){
        requestShutdown = true
    }
}