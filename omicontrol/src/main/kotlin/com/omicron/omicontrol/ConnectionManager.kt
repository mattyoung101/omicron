package com.omicron.omicontrol

import RemoteDebug
import com.google.protobuf.InvalidProtocolBufferException
import tornadofx.runLater
import java.net.InetSocketAddress
import java.net.Socket
import java.net.SocketException
import kotlin.concurrent.thread

class ConnectionManager {
    private var socket = Socket()
    private var requestShutdown = false
    private var tcpThread = thread(start=false){}

    // code run by tcpThread, separated so it can be restarted
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

            // FIXME this sucks
            var caughtException = false
            var msgNull = false
            try {
                val msg = RemoteDebug.DebugFrame.parseDelimitedFrom(stream)
                if (msg == null){
                    msgNull = true
                } else {
                    EVENT_BUS.post(msg)
                }
            } catch (e: SocketException){
                caughtException = true
                e.printStackTrace()
            } catch (e: InvalidProtocolBufferException){
                // was probably meant for remote debug so just ignore it
            }

            if (msgNull || caughtException){
                println("Remote has likely disconnected")
                EVENT_BUS.post(RemoteShutdownEvent())
                Thread.currentThread().interrupt()
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

    /**
     * Encode a DebugCommand Protocol Buffer and dispatch it to Omicam
     * @param onSuccess callback to run if command was successfully received, executed in JavaFX application thread
     * @param onError callback to run if an error occurred, optional, executed in JavaFX application thread
     */
    fun encodeAndSend(command: RemoteDebug.DebugCommand, onSuccess: (RemoteDebug.DebugCommand) -> Unit, onError: () -> Unit = {}){
        // TODO could we get into a situation where we accidentally receive the frame message as a response instead of the command?
        // TODO yo yo yo why don't we use a different socket address? like 42709 is the remote debug socket???
        // could also just not be a moron and frame the messages in a header
        // this is because Omicam checks for messages _AFTER_ dispatching the frame thread! maybe we need to read first!
        thread {
            println("Dispatching command to Omicam")
            val outStream = socket.getOutputStream()
            command.writeDelimitedTo(outStream)
            outStream.flush()
            println("Dispatched, awaiting response")

            for (i in 0..25) { // HACK HACK HACK
                try {
                    val response = RemoteDebug.DebugCommand.parseDelimitedFrom(socket.getInputStream())
                    if (response != null) {
                        println("Command response is OK!")
                        runLater { onSuccess(response) }
                        break
                    } else {
                        println("Command response is null!")
                        runLater { onError() }
                        break
                    }
                } catch (e: InvalidProtocolBufferException){
                    println("ignoring exception")
                }

                // unable to get a good message
                runLater { onError() }
            }
        }
    }

    fun disconnect(){
        requestShutdown = true
    }
}