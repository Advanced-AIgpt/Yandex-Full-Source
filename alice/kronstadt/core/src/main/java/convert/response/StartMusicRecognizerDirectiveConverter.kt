package ru.yandex.alice.kronstadt.core.convert.response

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.directive.StartMusicRecognizerDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TStartMusicRecognizerDirective

@Component
open class StartMusicRecognizerDirectiveConverter : DirectiveConverterBase<StartMusicRecognizerDirective> {
    override fun convert(src: StartMusicRecognizerDirective, ctx: ToProtoContext): TDirective {
        return TDirective.newBuilder()
            .setStartMusicRecognizerDirective(
                TStartMusicRecognizerDirective.newBuilder()
                    .setName("external_skill__start_music_recognizer")
                    .build()
            )
            .build()
    }

    override val directiveType: Class<StartMusicRecognizerDirective>
        get() = StartMusicRecognizerDirective::class.java
}
