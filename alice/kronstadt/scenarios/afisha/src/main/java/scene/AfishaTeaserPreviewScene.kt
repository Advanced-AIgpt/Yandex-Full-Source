package ru.yandex.alice.kronstadt.scenarios.afisha.scene

import org.apache.logging.log4j.LogManager
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.BaseRunResponse
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.ScenePrepareBuilder
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoContext
import ru.yandex.alice.kronstadt.core.domain.scenariodata.teasers.AfishaTeaserScenarioData
import ru.yandex.alice.kronstadt.core.domain.scenariodata.teasers.TeaserConfigData
import ru.yandex.alice.kronstadt.core.domain.converters.TeaserPreviewDataConverter
import ru.yandex.alice.kronstadt.core.domain.scenariodata.teasers.TeasersPreviewData
import ru.yandex.alice.kronstadt.core.layout.Layout
import ru.yandex.alice.kronstadt.core.scenario.AbstractNoargScene
import ru.yandex.alice.kronstadt.core.scenario.SceneWithPrepare
import ru.yandex.alice.kronstadt.scenarios.afisha.AfishaGraphQLService
import ru.yandex.alice.kronstadt.scenarios.afisha.AfishaTeaserService
import ru.yandex.alice.kronstadt.scenarios.afisha.model.response.AfishaEvent
import ru.yandex.alice.protos.data.scenario.Data

@Component
class AfishaTeaserPreviewScene(
    private val afishaGraphQLService: AfishaGraphQLService,
    private val afishaTeaserService: AfishaTeaserService,
    private val teaserPreviewDataConverter: TeaserPreviewDataConverter
) : AbstractNoargScene<Any>("afisha_teaser_preview_scene"), SceneWithPrepare<Any, Any> {
    override fun prepareRun(request: MegaMindRequest<Any>, args: Any, responseBuilder: ScenePrepareBuilder) {
        val httpRequest = afishaGraphQLService.buildRequest(request)
        responseBuilder.addHttpRequest(AFISHA_MM_REQUEST, httpRequest)
    }

    override fun render(request: MegaMindRequest<Any>): BaseRunResponse<Any>? {
        val recommendationEvent = afishaTeaserService.getRecommendationEvents(request, 1).get(0)
        val teaserPreviewData = TeasersPreviewData(
            listOf(
                TeasersPreviewData.TeaserPreview(
                    teaserConfigData = TeaserConfigData(AFISHA_TEASER_TYPE),
                    teaserName = AFISHA_TEASER_NAME,
                    previewScenarioData = afishaTeaserService.getAfishaScenarioData(recommendationEvent)
                )
            )
        )

        return RunOnlyResponse(
            layout = Layout(shouldListen = false, outputSpeech = null),
            state = null,
            analyticsInfo = AnalyticsInfo(intent = GET_PREVIEW_INTENT),
            isExpectsRequest = false,
            scenarioData = Data.TScenarioData.newBuilder()
                .setTeasersPreviewData(teaserPreviewDataConverter.convert(teaserPreviewData, ToProtoContext()))
                .build()

        )
    }

    companion object {
        private val logger = LogManager.getLogger(AfishaTeaserScene::class.java)
        private const val AFISHA_MM_REQUEST: String = "afisha_mm_request"
        private const val AFISHA_TEASER_TYPE = "Afisha";
        private const val AFISHA_TEASER_NAME = "Афиша";
        private const val GET_PREVIEW_INTENT = "alice_scenarios.get_afisha_preview"
    }
}
