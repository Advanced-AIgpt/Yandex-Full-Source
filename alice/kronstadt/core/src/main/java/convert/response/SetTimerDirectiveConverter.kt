package ru.yandex.alice.kronstadt.core.convert.response

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.directive.SetTimerDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TSetTimerDirective

@Component
open class SetTimerDirectiveConverter : DirectiveConverterBase<SetTimerDirective> {
    override val directiveType: Class<SetTimerDirective>
        get() = SetTimerDirective::class.java

    override fun convert(src: SetTimerDirective, ctx: ToProtoContext): TDirective {
        val setTimer: TSetTimerDirective = TSetTimerDirective.newBuilder()
            .setDuration(src.duration.toSeconds())
            .setListeningIsPossible(src.isListeningIsPossible)
            .build()
        return TDirective.newBuilder()
            .setSetTimerDirective(setTimer)
            .build()
    }
}
