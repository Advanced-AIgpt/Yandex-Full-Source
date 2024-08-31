package ru.yandex.alice.kronstadt.core.convert.response

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.directive.StopPlayingTimerDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TTimerStopPlayingDirective

@Component
open class StopCurrentlyPlayingTimerDirectiveConverter : DirectiveConverterBase<StopPlayingTimerDirective> {
    override val directiveType: Class<StopPlayingTimerDirective>
        get() = StopPlayingTimerDirective::class.java

    override fun convert(src: StopPlayingTimerDirective, ctx: ToProtoContext): TDirective {
        val directive: TTimerStopPlayingDirective = TTimerStopPlayingDirective.newBuilder()
            .setTimerId(src.timerId)
            .build()
        return TDirective.newBuilder()
            .setTimerStopPlayingDirective(directive)
            .build()
    }
}
