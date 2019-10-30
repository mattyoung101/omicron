import kotlin.concurrent.thread
import kotlin.system.exitProcess

object Main {
    private val connectionManager = ConnectionManager()

    @JvmStatic
    fun main(args: Array<String>){
        Runtime.getRuntime().addShutdownHook(thread(start=false){
            println("Shutdown hook running")
            connectionManager.disconnect()
        })
        connectionManager.connect()

        while (true){}
    }
}