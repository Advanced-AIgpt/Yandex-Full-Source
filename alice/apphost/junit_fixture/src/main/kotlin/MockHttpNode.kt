package ru.yandex.alice.apphost

import NAppHostHttp.Http
import com.sun.net.httpserver.HttpExchange
import com.sun.net.httpserver.HttpHandler
import com.sun.net.httpserver.HttpServer
import java.net.InetSocketAddress

interface MockHttpHandler: HttpHandler {
    fun enqueue(response: Http.THttpResponse)
    fun reset()
}

private val DEFAULT_RESPONSE: Http.THttpResponse = Http.THttpResponse.newBuilder()
    .setStatusCode(500)
    .build()

class SingleResponseHttpHandler(
    private var response: Http.THttpResponse = DEFAULT_RESPONSE
): MockHttpHandler {

    override fun enqueue(response: Http.THttpResponse) {
        this.response = response
    }

    override fun reset() {
        response = DEFAULT_RESPONSE
    }

    override fun handle(exchange: HttpExchange) {
        exchange.sendResponseHeaders(response.statusCode, response.content.size().toLong())
        val outputStream = exchange.responseBody
        outputStream.write(response.content.toByteArray())
        outputStream.close()
    }

}

class MockHttpNode(
    private val handler: MockHttpHandler
) {
    private val server: HttpServer

    val port: Int
        get() = server.address.port

    init {
        server = HttpServer.create(InetSocketAddress(0), 0)
        server.createContext("/", handler)
        server.setExecutor(null)
        server.start()
    }

    fun reset() {
        handler.reset()
    }
}
