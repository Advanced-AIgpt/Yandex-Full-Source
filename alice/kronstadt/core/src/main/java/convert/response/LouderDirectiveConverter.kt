package ru.yandex.alice.kronstadt.core.convert.response

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.directive.SoundLouderDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TSoundLouderDirective

@Component
open class LouderDirectiveConverter : DirectiveConverterBase<SoundLouderDirective> {
    override fun convert(src: SoundLouderDirective, ctx: ToProtoContext): TDirective {
        val directive: TSoundLouderDirective.Builder = TSoundLouderDirective.newBuilder()
            .setName("external_skill__sound_louder")
        return TDirective.newBuilder()
            .setSoundLouderDirective(directive)
            .build()
    }

    override val directiveType: Class<SoundLouderDirective>
        get() = SoundLouderDirective::class.java
}
