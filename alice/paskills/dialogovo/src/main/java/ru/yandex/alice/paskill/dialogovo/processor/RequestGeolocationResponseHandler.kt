package ru.yandex.alice.paskill.dialogovo.processor

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.ActionRef.Companion.noEffect
import ru.yandex.alice.kronstadt.core.ActionRef.NluHint
import ru.yandex.alice.kronstadt.core.domain.Voice
import ru.yandex.alice.kronstadt.core.layout.TextCard
import ru.yandex.alice.kronstadt.core.text.Phrases
import ru.yandex.alice.paskill.dialogovo.domain.Experiments
import ru.yandex.alice.paskill.dialogovo.domain.RequestGeolocationButton
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessRequest
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessResult
import ru.yandex.alice.paskill.dialogovo.external.v1.response.WebhookResponse
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context
import ru.yandex.alice.paskill.dialogovo.processor.request_enrichment.RequestEnrichmentData
import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticFrames
import java.util.Optional

@Component
class RequestGeolocationResponseHandler(private val phrases: Phrases) : WebhookResponseHandler {
    override fun handleResponse(
        builder: SkillProcessResult.Builder,
        request: SkillProcessRequest,
        context: Context,
        requestEnrichment: RequestEnrichmentData,
        response: WebhookResponse
    ): SkillProcessResult.Builder {
        val testIntervalsEnabled = request.hasExperiment(Experiments.GEOSHARING_ENABLE_TEST_INTERVALS)
        val buttons = GeolocationSharingOptions.values()
            .filter { opt: GeolocationSharingOptions -> testIntervalsEnabled || opt.isAvailableInProduction }
            .map { RequestGeolocationButton(it.title, it.periodForAllowedSharing) }

        val askForPermission = phrases.getRandom("geolocation_sharing.ask_for_permission", request.random)
        val textCard = TextCard(
            text = askForPermission,
            buttons = buttons,
        )

        builder.getLayout()
            .textCard(textCard)
            .shouldListen(request.isVoiceSession)

        return builder
            .appendTts(
                if (request.isVoiceSession) askForPermission else "",
                Voice.SHITOVA_US,
                "\nsil<[200]> "
            ).nluHint(
                "allow_geosharing",
                noEffect(
                    NluHint(SemanticFrames.ALICE_EXTERNAL_SKILL_ALLOW_GEOSHARING)
                )
            ).nluHint(
                "do_not_allow_geosharing",
                noEffect(
                    NluHint(SemanticFrames.ALICE_EXTERNAL_SKILL_DO_NOT_ALLOW_GEOSHARING)
                )
            )
            .requestedGeolocation(Optional.of(true))
    }
}
