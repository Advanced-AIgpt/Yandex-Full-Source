package ru.yandex.alice.social.sharing.apphost.handlers

import NAppHostHttp.Http

fun header(name: String, value: String): Http.THeader {
    return Http.THeader.newBuilder()
        .setName(name)
        .setValue(value)
        .build()
}

private fun contentType(value: String) = header("Content-Type", value)

internal val DEFAULT_HEADERS_JSON: List<Http.THeader> = listOf(
    contentType("application/json"),
)

internal val DEFAULT_HEADERS_HTML: List<Http.THeader> = listOf(
    contentType("text/html"),
)

internal val REQUEST_HEADERS_PROTOBUF: List<Http.THeader> = listOf(
    contentType("application/x-protobuf"),
    header("Accept", "application/x-protobuf"),
)

fun List<Http.THeader>.getHeader(name: String): Http.THeader? {
    return this.firstOrNull { it.name.lowercase() == name.lowercase() }
}

fun List<Http.THeader>.getContentType(): String? {
    return this.getHeader("Content-Type")?.value?.lowercase()
}
