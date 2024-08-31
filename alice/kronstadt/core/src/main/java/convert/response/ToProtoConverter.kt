package ru.yandex.alice.kronstadt.core.convert.response

import com.google.protobuf.MessageOrBuilder

interface ToProtoConverter</*in */TSource, out TResult : MessageOrBuilder> {
    fun convert(src: TSource, ctx: ToProtoContext): TResult
}
