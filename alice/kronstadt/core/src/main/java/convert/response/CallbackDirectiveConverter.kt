package ru.yandex.alice.kronstadt.core.convert.response

import com.fasterxml.jackson.databind.ObjectMapper
import com.fasterxml.jackson.databind.node.ObjectNode
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.convert.ProtoUtil
import ru.yandex.alice.kronstadt.core.directive.CallbackDirective
import ru.yandex.alice.kronstadt.core.directive.Directive
import ru.yandex.alice.megamind.protos.scenarios.directive.TCallbackDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TDirective

@Component
class CallbackDirectiveConverter(
    private val protoUtil: ProtoUtil,
    private val objectMapper: ObjectMapper
) : DirectiveConverterBase<CallbackDirective> {

    override fun convert(src: CallbackDirective, ctx: ToProtoContext): TDirective {
        return TDirective.newBuilder()
            .setCallbackDirective(convert(src))
            .build()
    }

    fun convert(src: CallbackDirective): TCallbackDirective.Builder {
        val directive: Directive = src.javaClass.getAnnotation(Directive::class.java)
            ?: throw RuntimeException("class ${src.javaClass.name} is not annotated with @DirectiveName")
        val payload: ObjectNode = objectMapper.valueToTree(src)

        @Suppress("DEPRECATION")
        return TCallbackDirective.newBuilder()
            .setName(directive.value)
            .setPayload(protoUtil.objectToStruct(payload))
            .setIgnoreAnswer(directive.ignoreAnswer)
    }

    override val directiveType: Class<CallbackDirective>
        get() = CallbackDirective::class.java
}
