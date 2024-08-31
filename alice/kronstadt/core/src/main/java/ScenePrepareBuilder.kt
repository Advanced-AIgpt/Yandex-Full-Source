package ru.yandex.alice.kronstadt.core

import ru.yandex.alice.paskills.common.apphost.http.HttpRequest
import ru.yandex.alice.paskills.common.apphost.http.HttpRequestConverter
import ru.yandex.web.apphost.api.request.ApphostResponseBuilder

interface ScenePrepareBuilder : ApphostResponseBuilder {
    fun addHttpRequest(type: String = "http_request", payload: HttpRequest<*>)
}

fun scenePrepareBuilder(context: ApphostResponseBuilder, converter: HttpRequestConverter): ScenePrepareBuilder =
    ScenePrepareBuilderImpl(context, converter)

internal class ScenePrepareBuilderImpl(
    private val delegate: ApphostResponseBuilder,
    private val converter: HttpRequestConverter
) : ApphostResponseBuilder by delegate, ScenePrepareBuilder {

    override fun addHttpRequest(type: String, payload: HttpRequest<*>) {
        delegate.addProtobufItem(type, converter.httpRequestToProto(payload))
    }
}
