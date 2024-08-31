package ru.yandex.alice.kronstadt.scenarios.tv_channels.indexer

import com.fasterxml.jackson.databind.ObjectMapper
import com.fasterxml.jackson.module.kotlin.readValue
import org.apache.logging.log4j.LogManager
import org.springframework.beans.factory.annotation.Value
import org.springframework.http.HttpEntity
import org.springframework.http.HttpHeaders
import org.springframework.http.HttpMethod
import org.springframework.http.MediaType
import org.springframework.web.client.HttpClientErrorException
import org.springframework.web.client.HttpStatusCodeException
import org.springframework.web.client.RestTemplate
import org.springframework.web.client.exchange
import java.net.URI
import java.util.concurrent.CompletableFuture
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors

class SaasIndexerClientImpl(
    // TODO: add solomon interceptors
    private val restTemplate: RestTemplate,
    private val objectMapper: ObjectMapper,
    @Value("\${tv-channel-indexer.saas-indexer.url}") url: String,
    private val executorService: ExecutorService = Executors.newCachedThreadPool(),
) : SaasIndexerClient {
    private val logger = LogManager.getLogger()
    private val saasUri: URI = URI.create(url)

    fun indexDocumentAsync(msg: SaasMessage): CompletableFuture<IndexerResponseItem?> {
        val headers = HttpHeaders()
        headers.add(HttpHeaders.CONTENT_TYPE, MediaType.APPLICATION_JSON_VALUE)
        return CompletableFuture.supplyAsync({ executeCall(msg, headers) }, executorService)
    }

    private fun executeCall(msg: SaasMessage, headers: HttpHeaders): IndexerResponseItem? {
        try {
            logger.debug("Call saas-daemon for ")
            val response = restTemplate.exchange<IndexerResponseItem>(
                saasUri,
                HttpMethod.POST,
                HttpEntity(msg, headers)
            )

            logger.debug("call response: {}", response.body)

            return response.body
        } catch (e: HttpClientErrorException.BadRequest) {
            logger.error("Saas indexer call failed due to Incorrect document", e)
            return objectMapper.readValue<IndexerResponseItem>(e.responseBodyAsString)
        } catch (e: HttpStatusCodeException) {
            logger.error("Http error on saas call", e)
            return objectMapper.readValue<IndexerResponseItem>(e.responseBodyAsString)
        }
    }

    override fun sendDocuments(msgs: List<SaasMessage>) {
        CompletableFuture.allOf(
            *msgs.map {
                indexDocumentAsync(it)
                    .exceptionally { e ->
                        logger.error("Failed to index document", e)
                        IndexerResponseItem("unknown error")
                    }
            }.toTypedArray()
        ).join()
    }
}
