package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.processors

import org.springframework.beans.factory.annotation.Qualifier
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.BaseRunResponse
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.layout.Layout.Companion.textWithOutputSpeech
import ru.yandex.alice.kronstadt.core.text.Phrases
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context
import ru.yandex.alice.paskill.dialogovo.megamind.processor.RunRequestProcessor
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.analyticsonly.SurfaceNotSupportedIntent

@Component
class UnsupportedSurfaceFallbackProcessor(@param:Qualifier("recipePhrases") private val phrases: Phrases) :
    RunRequestProcessor<DialogovoState> {
    override fun canProcess(request: MegaMindRequest<DialogovoState>): Boolean =
        request.clientInfo.isNavigatorOrMaps || request.clientInfo.isYaAuto

    override val type: RunRequestProcessorType = RunRequestProcessorType.UNSUPPORTED_SURFACE

    override fun process(context: Context, request: MegaMindRequest<DialogovoState>): BaseRunResponse<DialogovoState> {
        val textWithTts = phrases.getRandomTextWithTts("surface_not_supported", request.random)

        val layout = textWithOutputSpeech(textWithTts, false)
        return RunOnlyResponse(
            layout = layout,
            state = request.state,
            analyticsInfo = context.analytics.toAnalyticsInfo(SurfaceNotSupportedIntent.analyticsInfoName),
            isExpectsRequest = false
        )
    }
}
