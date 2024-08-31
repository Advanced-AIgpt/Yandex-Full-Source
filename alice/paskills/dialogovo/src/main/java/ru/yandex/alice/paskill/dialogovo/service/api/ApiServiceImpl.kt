package ru.yandex.alice.paskill.dialogovo.service.api

import org.apache.logging.log4j.LogManager
import org.springframework.http.HttpEntity
import org.springframework.http.HttpHeaders
import org.springframework.http.HttpMethod
import org.springframework.stereotype.Component
import org.springframework.util.MimeTypeUtils
import org.springframework.web.client.HttpClientErrorException
import org.springframework.web.client.RestTemplate
import org.springframework.web.client.exchange
import org.springframework.web.util.UriComponentsBuilder
import ru.yandex.alice.paskill.dialogovo.config.ApiConfig
import ru.yandex.alice.paskill.dialogovo.domain.FeedbackMark
import ru.yandex.alice.paskills.common.resttemplate.factory.RestTemplateClientFactory
import ru.yandex.alice.paskills.common.tvm.spring.handler.SecurityHeaders
import ru.yandex.passport.tvmauth.TvmClient
import java.time.Duration

private const val UUID_HEADER = "X-UUID"

@Component
internal open class ApiServiceImpl(
    restTemplateClientFactory: RestTemplateClientFactory,
    val tvmClient: TvmClient,
    private val endpointConfig: ApiConfig
) : ApiService {

    private val webClient: RestTemplate = restTemplateClientFactory
        .serviceWebClientWithRetry(
            "api",
            Duration.ofMillis(400),
            2, true,
            Duration.ofMillis(endpointConfig.connectTimeout.toLong()),
            Duration.ofMillis(endpointConfig.timeout.toLong()),
            100
        )

    private val logger = LogManager.getLogger()

    override fun getFeedbackMark(userTicket: String, skillId: String, deviceUuid: String): FeedbackMark? {
        val url = UriComponentsBuilder.fromUriString(endpointConfig.url)
            .pathSegment("reviews", "{skillId}", "my")
            .build(skillId)

        return try {
            val entity = getHttpEntity(userTicket, deviceUuid)
            logger.debug("GET {}", url)

            return webClient.exchange<GetReviewResponse?>(url, HttpMethod.GET, entity)
                .body?.result?.let { FeedbackMark.ofValue(it.rating) }
        } catch (e: HttpClientErrorException.NotFound) {
            logger.info("No review for skill {}", skillId)
            null
        } catch (e: Exception) {
            throw ApiException("Unable to obtain skill review", e)
        }
    }

    override fun putFeedbackMark(userTicket: String, skillId: String, mark: FeedbackMark, deviceUuid: String) {
        val url = UriComponentsBuilder.fromUriString(endpointConfig.url)
            .pathSegment("reviews", "{skillId}")
            .build(skillId)

        val payload = ReviewPayload(rating = mark.markValue)

        try {
            val entity = postHttpEntity(userTicket, payload, deviceUuid)
            logger.debug("POST {}", url)

            webClient.exchange<ReviewPayload>(url, HttpMethod.POST, entity)
        } catch (e: Exception) {
            throw ApiException("Unable to save skill review", e)
        }
    }

    private fun getHttpEntity(userTicket: String, deviceUuid: String): HttpEntity<Void> {
        val headers = generateHttpHeaders(userTicket, deviceUuid)
        return HttpEntity(headers)
    }

    private fun postHttpEntity(
        userTicket: String,
        payload: ReviewPayload,
        deviceUuid: String
    ): HttpEntity<ReviewPayload> {
        val headers = generateHttpHeaders(userTicket, deviceUuid)
        return HttpEntity(payload, headers)
    }

    private fun generateHttpHeaders(userTicket: String, deviceUuid: String): HttpHeaders {
        val ticket = tvmClient.getServiceTicketFor("api")
        val headers = HttpHeaders()
        headers.add(SecurityHeaders.SERVICE_TICKET_HEADER, ticket)
        headers.add(SecurityHeaders.USER_TICKET_HEADER, userTicket)
        headers.add(UUID_HEADER, deviceUuid)
        headers.add(HttpHeaders.CONTENT_TYPE, MimeTypeUtils.APPLICATION_JSON_VALUE)
        return headers
    }

    private data class GetReviewResponse(
        val result: ReviewPayload?
    )

    private data class ReviewPayload(
        val rating: Int,
        val reviewText: String = "",
        val quickAnswers: List<String> = listOf(),
    )
}
