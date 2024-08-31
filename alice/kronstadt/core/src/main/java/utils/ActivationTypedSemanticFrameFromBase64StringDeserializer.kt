package ru.yandex.alice.kronstadt.core.utils

import com.fasterxml.jackson.core.JsonParser
import com.fasterxml.jackson.databind.DeserializationContext
import com.fasterxml.jackson.databind.JsonDeserializer
import ru.yandex.alice.megamind.protos.common.FrameProto.TTypedSemanticFrame
import java.util.Base64

// Used only for CallbackDirective
class ActivationTypedSemanticFrameFromBase64StringDeserializer : JsonDeserializer<TTypedSemanticFrame>() {

    override fun deserialize(parser: JsonParser, context: DeserializationContext): TTypedSemanticFrame =
        TTypedSemanticFrame.parseFrom(Base64.getDecoder().decode(parser.text))
}
