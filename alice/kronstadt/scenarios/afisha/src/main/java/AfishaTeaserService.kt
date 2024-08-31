package ru.yandex.alice.kronstadt.scenarios.afisha

import org.apache.logging.log4j.LogManager
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.domain.scenariodata.teasers.AfishaTeaserScenarioData
import ru.yandex.alice.kronstadt.scenarios.afisha.model.response.AfishaEvent
import ru.yandex.alice.kronstadt.scenarios.afisha.model.response.Items
import ru.yandex.alice.kronstadt.scenarios.afisha.model.response.Response
import ru.yandex.alice.kronstadt.scenarios.afisha.scene.AfishaTeaserScene

@Component
class AfishaTeaserService {

    private fun getResponse(request: MegaMindRequest<Any>): Map<String, Items> {
        val afishaHttpResponse = request.additionalSources
            .getSingleHttpResponse<Response>(AFISHA_MM_RESPONSE)
            ?: error("No ${AFISHA_MM_RESPONSE} in context")
        if (!afishaHttpResponse.is2xxSuccessful) {
            error("Request to Afisha filed with  status code: ${afishaHttpResponse.statusCode}")
        }
        val afishaGraphQlResponse = afishaHttpResponse.content
            ?: error("Empty content from Afisha GraphQL")
        if (afishaGraphQlResponse.errors.isNotEmpty()) {
            throw AfishaGraphQLException(afishaGraphQlResponse.errors)
        }
        return afishaGraphQlResponse.data ?: error("No data in Response from GraphQL")
    }

    fun getRecommendationEvents(request: MegaMindRequest<Any>, maxNumberOfTeasers: Int): ArrayList<AfishaEvent> {
        val events: Map<String, Items> = getResponse(request)
        val recommendationEvents = ArrayList<AfishaEvent>()
        events[RECOMMENDED_CONCERT_EVENTS]?.items?.let { recommendationEvents.addAll(it) }
        events[RECOMMENDED_THEATRE_EVENTS]?.items?.let { recommendationEvents.addAll(it) }
        if (recommendationEvents.size < maxNumberOfTeasers) {
            logger.debug(
                "Got ${recommendationEvents.size} recommendations from Afisha for user ${request.clientInfo.uuid}"
            )
            recommendationEvents.addAll(
                events[ACTUAL_EVENTS]?.items
                    ?.shuffled()
                    ?.take(maxNumberOfTeasers - recommendationEvents.size)
                    ?: error("No actual events from Afisha")
            )
        }
        return recommendationEvents
    }

    fun getAfishaScenarioData(afishaEvent: AfishaEvent): AfishaTeaserScenarioData {
        val event = afishaEvent.event
        val scheduleInfo = afishaEvent.scheduleInfo
        return AfishaTeaserScenarioData(
            title = event.title,
            imageUrl = event.image?.image?.url,
            date = scheduleInfo?.datePreview?.date,
            place = scheduleInfo?.placePreview,
            contentRating = event.contentRating
        )
    }

    companion object {
        private val logger = LogManager.getLogger(AfishaTeaserScene::class.java)
        private const val AFISHA_MM_RESPONSE: String = "afisha_mm_response"
        private const val RECOMMENDED_CONCERT_EVENTS = "recommendedConcertEvents"
        private const val RECOMMENDED_THEATRE_EVENTS = "recommendedTheatreEvents"
        private const val ACTUAL_EVENTS = "actualEvents"
    }
}
