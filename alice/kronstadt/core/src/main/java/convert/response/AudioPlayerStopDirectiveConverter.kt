package ru.yandex.alice.kronstadt.core.convert.response

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.directive.AudioPlayerStopDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TAudioStopDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TDirective

@Component
open class AudioPlayerStopDirectiveConverter : DirectiveConverterBase<AudioPlayerStopDirective> {
    override val directiveType: Class<AudioPlayerStopDirective>
        get() = AudioPlayerStopDirective::class.java

    override fun convert(src: AudioPlayerStopDirective, ctx: ToProtoContext): TDirective {
        val protoDirective = TAudioStopDirective.newBuilder()
            .setName("audio_stop")
            .setSmooth(true)

        return TDirective.newBuilder()
            .setAudioStopDirective(protoDirective)
            .build()
    }
}
