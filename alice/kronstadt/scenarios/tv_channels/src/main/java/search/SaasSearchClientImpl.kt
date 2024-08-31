package ru.yandex.alice.kronstadt.scenarios.tv_channels.search

import org.apache.logging.log4j.LogManager
import org.springframework.http.HttpHeaders
import org.springframework.http.MediaType
import org.springframework.http.RequestEntity
import org.springframework.http.ResponseEntity
import org.springframework.web.client.HttpStatusCodeException
import org.springframework.web.client.RestTemplate
import org.springframework.web.client.exchange
import org.springframework.web.util.UriComponentsBuilder

class SaasSearchClientImpl(
    private val client: RestTemplate,
    url: String,
    timeoutMs: Long,
    private val serviceName: String,
) : SaasSearchClient {

    private val urlBuilder: UriComponentsBuilder = UriComponentsBuilder.fromUriString(url)
        .queryParam("service", serviceName)
        .queryParam("format", "json")
        .queryParam("relev", "attr_limit=99999999")
        .queryParam("numdoc", "200")
        .queryParam("timeout", timeoutMs * 1000) // microseconds

    private val logger = LogManager.getLogger()

    override fun createSearchRequest(searchQuery: String): RequestEntity<*> {
        val url = urlBuilder.cloneBuilder()
            .queryParam("text", searchQuery)
            .queryParam("kps", 1)
            .build()
            .toUri()

        logger.info("Calling SaaS with url: ${url.toASCIIString()}")
        return RequestEntity.get(url)
            .header(HttpHeaders.ACCEPT, MediaType.APPLICATION_JSON_VALUE)
            .build()
    }

    override fun parseResult(response: ResponseEntity<SearchResponse>): List<FoundDocument> {
        return response.body?.response
            ?.results
            ?.firstOrNull()
            ?.also { result -> logger.info("Found ${result.found?.all} documents") }
            ?.takeIf { result -> (result.found?.all ?: 0) > 0 }
            ?.groups
            ?.flatMap { it.documents }
            ?.filter { it.url != null && it.properties?.deviceId != null }
            ?.map { doc ->
                FoundDocument(
                    url = doc.url!!,
                    deviceId = doc.properties?.deviceId!!,
                    timestamp = doc.properties.ts
                )
            }
            ?: listOf<FoundDocument>().also { logger.info("No documents found") }
    }

    override fun search(searchQuery: String): List<FoundDocument> {
        val request = createSearchRequest(searchQuery)
        try {
            logger.info("Search in SaaS with request: ${request.url}")
            val response = client.exchange<SearchResponse>(request)
            return parseResult(response)
        } catch (e: HttpStatusCodeException) {
            logger.error("SaaS search request failed", e)
            return listOf()
        }
    }
}
