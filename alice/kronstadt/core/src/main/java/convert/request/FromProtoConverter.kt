package ru.yandex.alice.kronstadt.core.convert.request

import com.google.protobuf.MessageOrBuilder

interface FromProtoConverter<TSource : MessageOrBuilder, TResult> {
    fun convert(src: TSource): TResult
}
