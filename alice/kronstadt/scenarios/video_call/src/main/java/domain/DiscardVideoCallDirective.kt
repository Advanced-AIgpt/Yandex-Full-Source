package ru.yandex.alice.kronstadt.scenarios.video_call.domain

import ru.yandex.alice.kronstadt.core.VideoCallCapability
import ru.yandex.alice.kronstadt.core.directive.MegaMindDirective
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrameRequestData

data class DiscardVideoCallDirective(
    val providerData: ProviderData,
) : MegaMindDirective {

    sealed class ProviderData

    data class TelegramDiscardVideoCallData(
        val callOwnerData: VideoCallCapability.TelegramVideoCallOwnerData,
        val onSuccessCallback: SemanticFrameRequestData? = null,
        val onFailCallback: SemanticFrameRequestData? = null,
    ) : ProviderData()
}