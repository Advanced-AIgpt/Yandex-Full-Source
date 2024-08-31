package ru.yandex.alice.kronstadt.core.convert.response

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.directive.SoundQuieterDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TSoundQuiterDirective

@Component
open class QuieterDirectiveConverter : DirectiveConverterBase<SoundQuieterDirective> {
    override fun convert(src: SoundQuieterDirective, ctx: ToProtoContext): TDirective {
        val directive: TSoundQuiterDirective.Builder = TSoundQuiterDirective.newBuilder()
            .setName("external_skill__sound_quiter")
        return TDirective.newBuilder()
            .setSoundQuiterDirective(directive)
            .build()
    }

    override val directiveType: Class<SoundQuieterDirective>
        get() = SoundQuieterDirective::class.java
}
