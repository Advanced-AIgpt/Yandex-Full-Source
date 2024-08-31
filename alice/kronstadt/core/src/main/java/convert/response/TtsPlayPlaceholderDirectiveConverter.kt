package ru.yandex.alice.kronstadt.core.convert.response

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.directive.TtsPlayPlaceholderDirective
import ru.yandex.alice.megamind.protos.common.DirectiveChannel
import ru.yandex.alice.megamind.protos.scenarios.directive.TDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TTtsPlayPlaceholderDirective

@Component
open class TtsPlayPlaceholderDirectiveConverter : DirectiveConverterBase<TtsPlayPlaceholderDirective> {
    override val directiveType: Class<TtsPlayPlaceholderDirective>
        get() = TtsPlayPlaceholderDirective::class.java

    override fun convert(src: TtsPlayPlaceholderDirective, ctx: ToProtoContext): TDirective {
        val builder = TTtsPlayPlaceholderDirective.newBuilder()

        src.channel?.also {
            builder.directiveChannel = when (src.channel) {
                TtsPlayPlaceholderDirective.DirectiveChannel.DIALOG -> DirectiveChannel.TDirectiveChannel.EDirectiveChannel.Dialog
                else -> DirectiveChannel.TDirectiveChannel.EDirectiveChannel.Content
            }
        }
        return TDirective.newBuilder()
            .setTtsPlayPlaceholderDirective(builder)
            .build()
    }
}
