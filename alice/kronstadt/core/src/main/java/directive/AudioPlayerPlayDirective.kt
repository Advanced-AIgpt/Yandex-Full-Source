package ru.yandex.alice.kronstadt.core.directive

import ru.yandex.alice.megamind.protos.scenarios.directive.TAudioPlayDirective.EScreenType
import ru.yandex.alice.megamind.protos.scenarios.directive.TAudioPlayDirective.TBackgroundMode
import ru.yandex.alice.megamind.protos.scenarios.directive.TAudioPlayDirective.TStream.TStreamFormat
import java.net.URI

data class AudioPlayerPlayDirective constructor(
    // name is used for player analytics
    val name: String,
    val playAction: Play,
    val providerName: String,
    val scenarioMeta: Map<String, String> = emptyMap(),
    val onPlayStartedCallback: CallbackDirective? = null,
    val onPlayStoppedCallback: CallbackDirective? = null,
    val onPlayFinishedCallback: CallbackDirective? = null,
    val onFailedCallback: CallbackDirective? = null,
    val screenType: EScreenType = EScreenType.Default,
) : MegaMindDirective {

    data class Play(
        val audioItem: AudioItem,
        val backgroundMode: TBackgroundMode,
    )

    data class AudioItem(
        val stream: AudioStream,
        val metadata: AudioMetadata? = null,
    )

    data class AudioStream @JvmOverloads constructor(
        val url: URI,
        val offsetMs: Int,
        val token: String,
        val streamFormat: TStreamFormat = TStreamFormat.MP3
    )

    data class AudioMetadata(
        val title: String? = null,
        val subTitle: String? = null,
        val art: Image? = null,
        val backgroundImage: Image? = null,
    ) {
        fun isEmpty() =
            title == null && subTitle == null && art == null && backgroundImage == null
    }

    data class Image(val url: URI)
}
