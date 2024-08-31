package ru.yandex.alice.paskills.ammo

import com.google.protobuf.Message
import java.io.OutputStream
import java.nio.charset.StandardCharsets

internal class AmmoGenerator(private val output: OutputStream) : IAmmoGenerator {
    private val requests: MutableList<Request> = mutableListOf()
    override fun get(path: String, tag: String, body: NoBodyRequestBuilder.() -> NoBodyRequestBuilder) {
        val request = Request(path = path, tag = tag, method = Method.GET)
        body.invoke(request)
        requests += request
    }

    override fun post(path: String, tag: String, body: RequestBuilder.() -> RequestBuilder) {
        val request = Request(path = path, tag = tag, method = Method.POST)
        body.invoke(request)
        requests += request
    }

    override fun put(path: String, tag: String, body: RequestBuilder.() -> RequestBuilder) {
        val request = Request(path = path, tag = tag, method = Method.PUT)
        body.invoke(request)
        requests += request
    }

    override fun delete(path: String, tag: String, body: NoBodyRequestBuilder.() -> NoBodyRequestBuilder) {
        val request = Request(path = path, tag = tag, method = Method.DELETE)
        body.invoke(request)
        requests += request
    }

    override fun request(path: String, tag: String, body: RequestBuilder.() -> RequestBuilder) {
        val request = Request(path = path, tag = tag)
        body.invoke(request)
        requests += request
    }

    fun flush() {
        val writer = output.buffered()

        requests.forEach {
            writer.write(it.flush())
            writer.write("\n".toByteArray(StandardCharsets.UTF_8))
        }
        writer.flush()
    }
}

internal class Request(
    private var path: String = "/",
    private var tag: String = "",
    private var version: Version = Version.HTTP_1_1,
    private var method: Method = Method.GET,
    private var body: ByteArray? = null
) : RequestBuilder {
    private val headers: MutableMap<String, String> = mutableMapOf()

    override fun path(path: String): Request {
        this.path = path
        return this
    }

    override fun tag(tag: String): Request {
        this.tag = tag
        return this
    }

    override fun method(method: Method): Request {
        this.method = method
        return this
    }

    override fun header(key: String, value: String): Request {
        this.headers[key] = value
        return this
    }

    override fun headers(vararg pairs: Pair<String, String>): Request {
        this.headers.putAll(pairs)
        return this
    }

    override fun body(body: String): Request = body(body.toByteArray(StandardCharsets.UTF_8))

    override fun body(message: Message): Request {
        return body(message.toByteArray())
    }

    override fun body(bytes: ByteArray): Request {
        if (bytes.isNotEmpty()) {
            headers["Content-Length"] = bytes.size.toString()
        }
        this.body = bytes
        return this
    }

    override fun version(version: Version): Request {
        this.version = version
        return this
    }

    fun flush(): ByteArray {
        val headersList = headers.entries
            .joinToString(separator = "\r\n") { (key, value) ->
                "$key: $value"
            } + if (headers.isEmpty()) "" else "\r\n"

        var bytes = "${method.name} $path ${version.code}\r\n$headersList\r\n".toByteArray(StandardCharsets.UTF_8) //+
        val tmpBody = body
        if (tmpBody != null) {
            bytes += tmpBody
        }

        return "${bytes.size} $tag\n".toByteArray(StandardCharsets.UTF_8) + bytes
    }
}
