import kotlin.concurrent.thread
import kotlin.system.exitProcess

object Main {

    @JvmStatic
    fun main(args: Array<String>){
        val connectionManager = ConnectionManager()
        connectionManager.connect()
    }
}