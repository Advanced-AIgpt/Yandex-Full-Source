package ru.yandex.alice.kronstadt.core.convert.response

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.directive.PlayerPauseDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TPlayerPauseDirective

@Component
open class PlayerPauseDirectiveConverter : DirectiveConverterBase<PlayerPauseDirective> {
    override val directiveType: Class<PlayerPauseDirective>
        get() = PlayerPauseDirective::class.java

    override fun convert(src: PlayerPauseDirective, ctx: ToProtoContext): TDirective {
        val directive: TPlayerPauseDirective.Builder = TPlayerPauseDirective.newBuilder()
            .setName("player_pause_directive")
            .setSmooth(src.isSmooth)
        return TDirective.newBuilder()
            .setPlayerPauseDirective(directive)
            .build()
    }
}
