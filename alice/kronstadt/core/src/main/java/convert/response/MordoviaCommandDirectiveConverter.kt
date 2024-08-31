package ru.yandex.alice.kronstadt.core.convert.response

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.convert.ProtoUtil
import ru.yandex.alice.kronstadt.core.directive.MordoviaCommandDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TMordoviaCommandDirective

@Component
open class MordoviaCommandDirectiveConverter(private val protoUtil: ProtoUtil) :
    DirectiveConverterBase<MordoviaCommandDirective> {
    override fun convert(src: MordoviaCommandDirective, ctx: ToProtoContext): TDirective {
        val directive: TMordoviaCommandDirective.Builder = TMordoviaCommandDirective.newBuilder()
            .setName("external_skill__mordovia_command")
            .setCommand(src.command)
            .setMeta(protoUtil.objectToStruct(src.meta))
            .setViewKey(src.viewKey)
        return TDirective.newBuilder()
            .setMordoviaCommandDirective(directive)
            .build()
    }

    override val directiveType: Class<MordoviaCommandDirective>
        get() = MordoviaCommandDirective::class.java
}
