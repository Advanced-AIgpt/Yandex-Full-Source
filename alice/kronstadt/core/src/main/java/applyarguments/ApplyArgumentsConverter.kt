package ru.yandex.alice.kronstadt.core.applyarguments

import com.google.protobuf.Message
import ru.yandex.alice.kronstadt.core.convert.request.FromProtoConverter
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoConverter

interface ApplyArgumentsConverter
    : FromProtoConverter<com.google.protobuf.Any, ApplyArguments>,
    ToProtoConverter<ApplyArguments, Message>
