package ru.yandex.alice.paskills.ammo

import com.google.protobuf.Message
import java.io.File
import java.io.OutputStream

enum class Method {
    GET, POST, DELETE, PUT
}

enum class Version(val code: String) {
    HTTP_1_0("HTTP/1.0"),
    HTTP_1_1("HTTP/1.1")
}

interface IAmmoGenerator {
    fun get(path: String = "/", tag: String = "", body: NoBodyRequestBuilder.() -> NoBodyRequestBuilder)
    fun post(path: String = "/", tag: String = "", body: RequestBuilder.() -> RequestBuilder)
    fun put(path: String = "/", tag: String = "", body: RequestBuilder.() -> RequestBuilder)
    fun delete(path: String = "/", tag: String = "", body: NoBodyRequestBuilder.() -> NoBodyRequestBuilder)
    fun request(path: String = "/", tag: String = "", body: RequestBuilder.() -> RequestBuilder)
}

interface NoBodyRequestBuilder {
    fun path(path: String): NoBodyRequestBuilder
    fun tag(tag: String): NoBodyRequestBuilder
    fun method(method: Method): NoBodyRequestBuilder
    fun header(key: String, value: String): NoBodyRequestBuilder
    fun headers(vararg pairs: Pair<String, String>): NoBodyRequestBuilder
    fun version(version: Version): NoBodyRequestBuilder
}

interface RequestBuilder : NoBodyRequestBuilder {
    override fun path(path: String): RequestBuilder
    override fun tag(tag: String): RequestBuilder
    override fun method(method: Method): RequestBuilder
    override fun header(key: String, value: String): RequestBuilder
    override fun headers(vararg pairs: Pair<String, String>): RequestBuilder
    override fun version(version: Version): RequestBuilder
    fun body(body: String): RequestBuilder
    fun body(message: Message): RequestBuilder
    fun body(bytes: ByteArray): RequestBuilder
}

fun generateToFile(fileName: String, body: (IAmmoGenerator).() -> Unit) = generateToFile(File(fileName), body)
fun generateToFile(file: File, body: (IAmmoGenerator).() -> Unit) {
    with(file.outputStream()) {
        generateToOutputStream(this, body)
    }
}

fun generateToStdOut(body: IAmmoGenerator.() -> Unit) = generateToOutputStream(System.out, body)

fun generateToOutputStream(output: OutputStream, body: IAmmoGenerator.() -> Unit) {
    val generator = AmmoGenerator(output)
    generator.apply(body)
    generator.flush()
}
