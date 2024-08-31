package ru.yandex.alice.paskill.dialogovo.service.recommender

import org.apache.logging.log4j.LogManager
import org.springframework.web.client.RestTemplate
import org.springframework.web.client.postForObject
import org.springframework.web.util.UriComponentsBuilder
import ru.yandex.monlib.metrics.labels.Labels
import ru.yandex.monlib.metrics.registry.MetricRegistry

private const val CLIENT_NAME = "dialogovo"
private val EMPTY_RESPONSE = RecommenderResponse("", "", listOf())

class RecommenderServiceImpl(
    private val client: RestTemplate,
    private val metricRegistry: MetricRegistry,
    private val recommenderUrl: String
) : RecommenderService {
    private val logger = LogManager.getLogger()

    override fun search(
        cardName: RecommenderCardName,
        query: String?,
        attributes: RecommenderRequestAttributes
    ): RecommenderResponse {
        logger.info("Recommender request with utterance=[{}] cardName=[{}]", query, cardName.code)
        metricRegistry.rate("recommender.request", Labels.of("card_name", cardName.code)).inc()

        return try {
            val uriBuilder = UriComponentsBuilder.fromUriString(recommenderUrl)
                .queryParam("client", CLIENT_NAME)
                .queryParam("card_name", cardName.code)

            query?.also { uriBuilder.queryParam("utterance", it) }
            val url = uriBuilder.build().toUri()

            logger.debug(
                "Recommender request url: [{}] attributes: [{}]",
                { url.toASCIIString() }, { attributes }
            )

            val response = client.postForObject<RecommenderResponse>(url, attributes)
            logger.debug("Recommender response: [{}]", response)
            logger.info(
                "Found [{}] results [{}:{}]", response.items.size, response.recommendationSource,
                response.recommendationType
            )

            if (response.items.isNotEmpty()) {
                logger.info(
                    "Found skill-ids: [{}]", response.items.joinToString(separator = ",") { it.skillId }
                )
            } else {
                metricRegistry.rate("recommender.empty_response", Labels.of("card_name", cardName.code)).inc()
            }

            response
        } catch (ex: Exception) {
            logger.error("Error while getting request from recommender service", ex)
            EMPTY_RESPONSE
        }
    }
}
