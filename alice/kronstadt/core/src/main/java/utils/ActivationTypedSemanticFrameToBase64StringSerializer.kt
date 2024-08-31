package ru.yandex.alice.kronstadt.core.utils

import com.fasterxml.jackson.core.JsonGenerator
import com.fasterxml.jackson.databind.JsonSerializer
import com.fasterxml.jackson.databind.SerializerProvider
import ru.yandex.alice.megamind.protos.common.FrameProto.TTypedSemanticFrame
import java.util.Base64

// Used only for CallbackDirective
class ActivationTypedSemanticFrameToBase64StringSerializer : JsonSerializer<TTypedSemanticFrame?>() {

    override fun serialize(value: TTypedSemanticFrame?, generator: JsonGenerator, serializers: SerializerProvider) {
        if (value != null) {
            generator.writeString(Base64.getEncoder().encodeToString(value.toByteArray()))
        }
    }
}
