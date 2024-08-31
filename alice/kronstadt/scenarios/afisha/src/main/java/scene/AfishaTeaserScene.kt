package ru.yandex.alice.kronstadt.scenarios.afisha.scene

import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.scenario.AbstractNoargScene
import ru.yandex.alice.kronstadt.core.scenario.SceneWithPrepare
import ru.yandex.alice.kronstadt.scenarios.afisha.AfishaGraphQLService
import org.springframework.stereotype.Component
import org.apache.logging.log4j.LogManager
import org.springframework.beans.factory.annotation.Value
import ru.yandex.alice.kronstadt.core.DivRenderData
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RelevantResponse
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.ScenePrepareBuilder
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoContext
import ru.yandex.alice.kronstadt.core.directive.AddGalleryCardDirective
import ru.yandex.alice.kronstadt.core.domain.converters.AfishaConverter
import ru.yandex.alice.kronstadt.core.domain.scenariodata.teasers.TeaserConfigData
import ru.yandex.alice.kronstadt.core.layout.Layout.Companion.directiveOnlyLayout
import ru.yandex.alice.kronstadt.scenarios.afisha.AfishaTeaserService
import ru.yandex.alice.kronstadt.scenarios.afisha.model.response.AfishaEvent
import ru.yandex.alice.protos.data.scenario.Data.TScenarioData


@Component
class AfishaTeaserScene(
    private val afishaGraphQLService: AfishaGraphQLService,
    private val afishaTeaserService: AfishaTeaserService,
    private val afishaConverter: AfishaConverter,
    @Value("\${teasers.max_number}") private val maxNumberOfTeasers: Int
) : AbstractNoargScene<Any>("afisha_teaser_scene"), SceneWithPrepare<Any, Any> {

    override fun prepareRun(request: MegaMindRequest<Any>, args: Any, responseBuilder: ScenePrepareBuilder) {
        val httpRequest = afishaGraphQLService.buildRequest(request)
        responseBuilder.addHttpRequest(AFISHA_MM_REQUEST, httpRequest)
    }

    override fun render(request: MegaMindRequest<Any>): RelevantResponse<Any> {
        val recommendationEvents = afishaTeaserService.getRecommendationEvents(request, maxNumberOfTeasers)
        val eventsMap: Map<String, AfishaEvent> = afishaCards(recommendationEvents.shuffled())

        return RunOnlyResponse(
            layout = directiveOnlyLayout(
                eventsMap.keys.map {
                    AddGalleryCardDirective(it, TeaserConfigData(AFISHA_TEASER_TYPE))
                }
            ),
            state = null,
            analyticsInfo = AnalyticsInfo(intent = "alice_centaur.afisha_teaser"),
            isExpectsRequest = false,
            renderData = eventsMap.map {
                DivRenderData(
                    cardId = it.key,
                    TScenarioData.newBuilder().setAfishaTeaserData(
                        afishaConverter.convert(afishaTeaserService.getAfishaScenarioData(it.value), ToProtoContext())
                    ).build()
                )
            }
        )
    }

    private fun afishaCards(events: List<AfishaEvent>): Map<String, AfishaEvent> {
        return events.take(maxNumberOfTeasers)
            .mapIndexed { index, item -> "$TEASER_AFISHA_CARD_ID_PREFIX.$index" to item }
            .associate { it }
    }

    companion object {
        private val logger = LogManager.getLogger(AfishaTeaserScene::class.java)
        private const val AFISHA_MM_REQUEST: String = "afisha_mm_request"
        private const val TEASER_AFISHA_CARD_ID_PREFIX = "afisha.teaser.card"
        private const val AFISHA_TEASER_TYPE = "Afisha";
    }

}

