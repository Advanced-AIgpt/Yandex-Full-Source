package ru.yandex.alice.kronstadt.core

import NAppHostHttp.Http.THttpResponse
import com.fasterxml.jackson.module.kotlin.jacksonObjectMapper
import com.google.protobuf.Message
import ru.yandex.alice.paskills.common.apphost.http.HttpRequestConverter
import ru.yandex.alice.paskills.common.apphost.http.HttpResponse

class AdditionalSources(
    val items: Map<String, List<(Message) -> Message>>,
    val converter: HttpRequestConverter,
) {
    inline fun <reified T : Message> getItems(itemType: String): List<T> {
        val defaultInstance = com.google.protobuf.Internal.getDefaultInstance(T::class.java)

        return items.getOrDefault(itemType, listOf()).map { it.invoke(defaultInstance) as T }
    }

    inline fun <reified T : Message> getSingleItem(itemType: String): T? {
        val defaultInstance = com.google.protobuf.Internal.getDefaultInstance(T::class.java)
        return items.getOrDefault(itemType, listOf()).firstOrNull()?.let { it.invoke(defaultInstance) as T }
    }

    inline fun <reified T> getSingleHttpResponse(itemType: String = "http_response"): HttpResponse<T>? {

        return items.getOrDefault(itemType, listOf()).firstOrNull()
            ?.let { it.invoke(THttpResponse.getDefaultInstance()) as THttpResponse }
            ?.let { converter.protoToHttpResponse(it, T::class.java) }
    }

    companion object {
        val EMPTY = AdditionalSources(mapOf(), HttpRequestConverter(jacksonObjectMapper()))
    }
}
