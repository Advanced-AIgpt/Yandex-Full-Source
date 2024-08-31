package ru.yandex.alice.kronstadt.scenarios.video_call.domain

import ru.yandex.alice.kronstadt.core.directive.MegaMindDirective
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrameRequestData

data class StartVideoCallDirective (
    val providerData: ProviderData,
) : MegaMindDirective {

    sealed class ProviderData

    data class TelegramStartVideoCallData(
        val id: String,
        val userId: String,
        val recipientUserId: String,
        val videoEnabled: Boolean = false,
        val onAcceptedCallback: SemanticFrameRequestData? = null,
        val onDiscardedCallback: SemanticFrameRequestData? = null,
        val onFailCallback: SemanticFrameRequestData? = null,
    ) : ProviderData()
}
