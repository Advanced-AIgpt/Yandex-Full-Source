package ru.yandex.alice.kronstadt.scenarios.video_call.domain

import ru.yandex.alice.kronstadt.core.directive.MegaMindDirective
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrameRequestData

data class StartVideoCallLoginDirective (
    val providerData: ProviderData,
) : MegaMindDirective {

    sealed class ProviderData

    data class TelegramStartLoginData(
        val id: String,
        val onFailCallback: SemanticFrameRequestData? = null,
    ) : ProviderData()
}
