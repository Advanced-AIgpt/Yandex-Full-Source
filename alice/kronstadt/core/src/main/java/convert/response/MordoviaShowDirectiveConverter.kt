package ru.yandex.alice.kronstadt.core.convert.response

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.directive.MordoviaShowDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TCallbackDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TMordoviaShowDirective

@Component
open class MordoviaShowDirectiveConverter : DirectiveConverterBase<MordoviaShowDirective> {

    override fun convert(src: MordoviaShowDirective, ctx: ToProtoContext): TDirective {
        val directive: TMordoviaShowDirective.Builder = TMordoviaShowDirective.newBuilder()
            .setName("external_skill__modrovia_show")
            .setUrl(src.url)
            .setIsFullScreen(src.isFullScreen)
            .setViewKey(src.viewKey)
            .setCallbackPrototype(
                @Suppress("DEPRECATION")
                TCallbackDirective.newBuilder()
                    .setName("mordovia_callback_directive")
                    .setIgnoreAnswer(false)
            )
        return TDirective.newBuilder()
            .setMordoviaShowDirective(directive)
            .build()
    }

    override val directiveType: Class<MordoviaShowDirective>
        get() = MordoviaShowDirective::class.java
}
