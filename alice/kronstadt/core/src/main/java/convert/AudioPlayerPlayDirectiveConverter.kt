package ru.yandex.alice.kronstadt.core.convert

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.convert.response.CallbackDirectiveConverter
import ru.yandex.alice.kronstadt.core.convert.response.DirectiveConverterBase
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoContext
import ru.yandex.alice.kronstadt.core.directive.AudioPlayerPlayDirective
import ru.yandex.alice.kronstadt.core.directive.AudioPlayerPlayDirective.AudioItem
import ru.yandex.alice.kronstadt.core.directive.AudioPlayerPlayDirective.AudioMetadata
import ru.yandex.alice.kronstadt.core.directive.AudioPlayerPlayDirective.AudioStream
import ru.yandex.alice.megamind.protos.scenarios.directive.TAudioPlayDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TAudioPlayDirective.TAudioPlayMetadata
import ru.yandex.alice.megamind.protos.scenarios.directive.TDirective

// TODO: move to common directives. generify domain [AudioPlayerPlayDirective] class
@Component
open class AudioPlayerPlayDirectiveConverter(
    private val callbackConverter: CallbackDirectiveConverter
) : DirectiveConverterBase<AudioPlayerPlayDirective> {

    override val directiveType: Class<AudioPlayerPlayDirective>
        get() = AudioPlayerPlayDirective::class.java

    override fun convert(src: AudioPlayerPlayDirective, ctx: ToProtoContext): TDirective {
        val playAction: AudioPlayerPlayDirective.Play = src.playAction
        val audioItem: AudioItem = playAction.audioItem
        val stream: AudioStream = audioItem.stream
        val metadataO: AudioMetadata? = audioItem.metadata?.let { if (it.isEmpty()) null else it }

        val protoDirective = TAudioPlayDirective.newBuilder()
            // Station uses this field for analytics by directive
            .setName(src.name)
            .setStream(
                TAudioPlayDirective.TStream.newBuilder()
                    .setId(stream.token)
                    .setUrl(stream.url.toASCIIString())
                    .setOffsetMs(stream.offsetMs)
                    .setStreamFormat(stream.streamFormat)
            )
            .setBackgroundMode(playAction.backgroundMode)
            .setScreenType(src.screenType)
            .setProviderName(src.providerName)
            .putAllScenarioMeta(src.scenarioMeta)

        metadataO?.also { metadata: AudioMetadata ->
            val metadataB = TAudioPlayMetadata.newBuilder()
            metadata.title?.also { value -> metadataB.title = value }
            metadata.subTitle?.also { value -> metadataB.subTitle = value }
            metadata.art?.also { art -> metadataB.artImageUrl = art.url.toASCIIString() }
            protoDirective.audioPlayMetadata = metadataB.build()
        }
        protoDirective.callbacks = TAudioPlayDirective.TCallbacks.newBuilder().apply {
            src.onPlayStartedCallback?.also { setOnPlayStartedCallback(callbackConverter.convert(it)) }
            src.onPlayStoppedCallback?.also { setOnPlayStoppedCallback(callbackConverter.convert(it)) }
            src.onPlayFinishedCallback?.also { setOnPlayFinishedCallback(callbackConverter.convert(it)) }
            src.onFailedCallback?.also { setOnFailedCallback(callbackConverter.convert(it)) }
        }.build()


        return TDirective.newBuilder()
            .setAudioPlayDirective(protoDirective)
            .build()
    }
}
