import java.net.InetSocketAddress
import java.net.Socket
import java.nio.ByteBuffer
import java.nio.channels.SocketChannel
import kotlin.concurrent.thread

class ConnectionManager {
    private val socket = Socket()
    private val recvThread = thread(start=false, name="TCPReceiveThread"){
        val buffer = ByteArray(512000) // 512 KB, current buffers are around 55 KB
        val stream = socket.getInputStream()

        while (true){
            if (Thread.interrupted()) return@thread

            val bytesReceived = stream.read(buffer)
            if (bytesReceived != -1)
                println("Received $bytesReceived bytes from socket")
        }
    }

    fun connect(){
        connect(Values.REMOTE_IP, Values.REMOTE_PORT)
    }

    fun connect(ip: String, port: Int){
        println("Connecting to remote")
        socket.connect(InetSocketAddress(ip, port))
        recvThread.start()
        println("Connection accepted")
    }

    fun disconnect(){
        recvThread.interrupt()
        Thread.sleep(250)
        socket.close()
    }
}