package ru.yandex.alice.paskill.dialogovo.processor

import org.springframework.beans.factory.annotation.Qualifier
import org.springframework.beans.factory.annotation.Value
import org.springframework.stereotype.Component
import ru.yandex.alice.paskill.dialogovo.domain.Experiments
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessRequest
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessResult
import ru.yandex.alice.paskill.dialogovo.external.v1.response.WebhookResponse
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context
import ru.yandex.alice.paskill.dialogovo.processor.request_enrichment.RequestEnrichmentData
import ru.yandex.alice.paskill.dialogovo.service.abuse.AbuseApplier
import ru.yandex.monlib.metrics.labels.Labels
import ru.yandex.monlib.metrics.registry.MetricRegistry

@Component
class WidgetGalleryResponseHandler(
    private val abuseApplier: AbuseApplier,
    @Value("\${abuseConfig.enabled}") private val abuseEnabled: Boolean,
    @Qualifier("externalMetricRegistry") private val externalMetricRegistry: MetricRegistry,
) : WebhookResponseHandler {
    override fun handleResponse(
        builder: SkillProcessResult.Builder,
        request: SkillProcessRequest,
        context: Context,
        requestEnrichment: RequestEnrichmentData,
        webhookResponse: WebhookResponse
    ): SkillProcessResult.Builder {
        if (abuseEnabled && !request.hasExperiment(Experiments.DONT_APPLY_ABUSE)) {
            abuseApplier.apply(webhookResponse)
        }
        val response = webhookResponse.response.orElseThrow {
            externalMetricRegistry.rate(
                "widget_gallery.request.rate",
                Labels.of("skill_id", request.skillId, "result", "failure")
            ).inc()
            RuntimeException("No response in WebHook response for skill with id ${request.skill.id}")
        }
        externalMetricRegistry.rate(
            "widget_gallery.request.rate",
            Labels.of("skill_id", request.skillId, "result", "success")
        ).inc()
        return builder.widgetItem(response.widgetGalleryMeta)
    }
}
