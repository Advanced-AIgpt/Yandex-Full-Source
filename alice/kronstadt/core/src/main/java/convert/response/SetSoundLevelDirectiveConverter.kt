package ru.yandex.alice.kronstadt.core.convert.response

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.directive.SetSoundLevelDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TSoundSetLevelDirective

@Component
open class SetSoundLevelDirectiveConverter : DirectiveConverterBase<SetSoundLevelDirective> {
    override fun convert(src: SetSoundLevelDirective, ctx: ToProtoContext): TDirective {
        val directive: TSoundSetLevelDirective.Builder = TSoundSetLevelDirective.newBuilder()
            .setName("external_skill__sound_set_level")
            .setNewLevel(src.level)
        return TDirective.newBuilder()
            .setSoundSetLevelDirective(directive)
            .build()
    }

    override val directiveType: Class<SetSoundLevelDirective>
        get() = SetSoundLevelDirective::class.java
}
