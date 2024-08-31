package ru.yandex.alice.kronstadt.core.convert.response

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.directive.AudioPlayerRewindDirective
import ru.yandex.alice.kronstadt.core.directive.AudioPlayerRewindDirective.Rewind
import ru.yandex.alice.kronstadt.core.directive.AudioPlayerRewindDirective.RewindType
import ru.yandex.alice.megamind.protos.scenarios.directive.TAudioRewindDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TDirective

@Component
open class AudioPlayerRewindDirectiveConverter : DirectiveConverterBase<AudioPlayerRewindDirective> {
    override val directiveType: Class<AudioPlayerRewindDirective>
        get() = AudioPlayerRewindDirective::class.java

    override fun convert(src: AudioPlayerRewindDirective, ctx: ToProtoContext): TDirective {
        val rewind: Rewind = src.rewind
        val protoDirective = TAudioRewindDirective.newBuilder()
            .setName("player_rewind")
            .setAmountMs(rewind.amountMs)
            .setType(convertRewindType(rewind.type))

        return TDirective.newBuilder()
            .setAudioRewindDirective(protoDirective)
            .build()
    }

    private fun convertRewindType(type: RewindType): TAudioRewindDirective.EType {
        return when (type) {
            RewindType.BACKWARD -> TAudioRewindDirective.EType.Backward
            RewindType.FORWARD -> TAudioRewindDirective.EType.Forward
            RewindType.ABSOLUTE -> TAudioRewindDirective.EType.Absolute
        }
    }
}
