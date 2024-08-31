package ru.yandex.alice.kronstadt.scenarios.tv_channels.vh

import org.apache.logging.log4j.LogManager
import org.springframework.http.HttpEntity
import org.springframework.http.HttpHeaders
import org.springframework.http.HttpMethod
import org.springframework.web.client.HttpStatusCodeException
import org.springframework.web.client.RestTemplate
import org.springframework.web.util.UriComponentsBuilder
import ru.yandex.alice.kronstadt.core.RequestContext
import ru.yandex.alice.kronstadt.scenarios.tv_channels.vh.response.PlayerResponse
import java.net.URI

class VhClient(
    private val client: RestTemplate,
    private val requestContext: RequestContext,
    private val baseUrl: URI,
) {

    private val logger = LogManager.getLogger()

    fun contentDetail(contentId: String): PlayerResponse {

        val url = UriComponentsBuilder.fromUri(baseUrl)
            .pathSegment("v23", "player", "{content_id}.json")
            .queryParam("service", "ya-tv-android")
            .queryParam("from", "tvandroid")
            .build(contentId)

        val headers = HttpHeaders()

        if (!requestContext.oauthToken.isNullOrEmpty()) {
            headers[HttpHeaders.AUTHORIZATION] = "OAuth ${requestContext.oauthToken}"
        }

        try {
            val response = client.exchange(url, HttpMethod.GET, HttpEntity<Any>(headers), PlayerResponse::class.java)
            return response.body ?: throw RuntimeException("No vh response")
        } catch (e: HttpStatusCodeException) {
            logger.error("VH content_detail request failed", e)
            throw e
        }
    }
}
