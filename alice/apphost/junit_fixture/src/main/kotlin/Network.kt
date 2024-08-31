package ru.yandex.alice.apphost

import java.io.IOException
import java.lang.Exception
import java.net.Inet6Address
import java.net.InetAddress
import java.net.ServerSocket
import java.net.Socket
import java.time.Duration
import java.time.Instant

class ServiceDidNotStartException(
    val serviceName: String,
    val port: Int,
    val timeSpent: Duration,
) : Exception("Service $serviceName did not start on port $port after $timeSpent")

class ProcessIdDownException(
    val serviceName: String
) : Exception("Process $serviceName is down")

internal fun waitPort(
    process: Process,
    serviceName: String,
    port: Int,
    timeout: Duration,
    host: String = "localhost",
    sleepBetweenAttempts: Duration = Duration.ofSeconds(5),
) {
    val startTime = Instant.now()
    while (Instant.now() < startTime.plus(timeout)) {
        if (!process.isAlive) {
            throw ProcessIdDownException(serviceName)
        }
        if (checkPort(host, port)) {
            return
        } else {
            Thread.sleep(sleepBetweenAttempts.toMillis())
        }
    }
    throw ServiceDidNotStartException(
        serviceName,
        port,
        Duration.between(startTime, Instant.now()),
    )
}

internal fun checkPort(host: String, port: Int): Boolean {
    try {
        Socket(host, port)
        return true
    } catch (e: IOException) {
        return false
    }
}

class CannotBindException: Exception("Cannot find free port range for apphost services")

private fun checkPortRange(basePort: Int, range: Int): Boolean {
    for (i in 1..range) {
        try {
            val clientSocket = Socket(InetAddress.getLocalHost(), basePort + i)
            clientSocket.close()
            // something is listening at this port
            return false
        } catch (e: IOException) {
            // port is free
        }
    }
    return true
}

/**
 * Picks a random free port such that all ports in [port, port + range] range a free
 */
internal fun pickRandomPortRange(range: Int, maxAttempts: Int = 20): Int {
    for (attempt in 0..maxAttempts) {
        val serverSocket = ServerSocket(0, 1000, Inet6Address.getLocalHost())
        if (checkPortRange(basePort = serverSocket.localPort, range)) {
            return serverSocket.localPort
        }
    }
    throw CannotBindException()
}
