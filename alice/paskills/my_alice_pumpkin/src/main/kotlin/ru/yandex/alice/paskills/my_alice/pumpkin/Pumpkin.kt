package ru.yandex.alice.paskills.my_alice.pumpkin

import NAppHostHttp.Http
import com.google.protobuf.ByteString
import org.apache.logging.log4j.LogManager
import org.springframework.beans.factory.annotation.Value
import org.springframework.http.HttpEntity
import org.springframework.http.HttpMethod
import org.springframework.http.ResponseEntity
import org.springframework.scheduling.annotation.Scheduled
import org.springframework.stereotype.Component
import org.springframework.web.bind.annotation.RequestMapping
import org.springframework.web.bind.annotation.RestController
import org.springframework.web.client.RestTemplate
import org.springframework.web.client.exchange
import ru.yandex.monlib.metrics.labels.Labels
import ru.yandex.monlib.metrics.primitives.Counter
import ru.yandex.monlib.metrics.primitives.Rate
import ru.yandex.monlib.metrics.registry.MetricRegistry
import java.time.Duration
import java.time.Instant
import javax.servlet.http.HttpServletRequest
import javax.servlet.http.HttpServletResponse

interface Pumpkin {
    fun get(): Http.THttpResponse
    fun age(): Duration
}

class RequestFailedException(val statusCode: Int): Exception()

@Component
class PumpkinImpl(
    @Value("\${pumpkin.source.url}") val sourceUrl: String,
    private val restTemplate: RestTemplate,
    metricRegistry: MetricRegistry
): Pumpkin {

    private val logger = LogManager.getLogger()

    // https://a.yandex-team.ru/arc/trunk/arcadia/apphost/lib/common/constants.h?rev=r7678270#L71
    private val headerBlacklist: Set<String> = setOf(
        "connection",
        "content-encoding",
        "transfer-encoding",
        "content-length",
        "x-yandex-req-id",
        "x-yandex-request-id",
    ).map { it.toLowerCase() }.toSet()

    @Volatile private var cachedResponse: Http.THttpResponse
    @Volatile private var lastUpdatedAt: Instant
    private val successCounter: Rate
    private val errorCounter: Rate

    init {
        this.cachedResponse = querySource()
        this.lastUpdatedAt = Instant.now();
        this.successCounter = metricRegistry.rate("pumpkin.update", Labels.of("status", "success"))
        this.errorCounter = metricRegistry.rate("pumpkin.update", Labels.of("status", "error"))
    }

    private fun querySource(): Http.THttpResponse {
        logger.debug("Querying pumpkin source {}", sourceUrl)
        val httpEntity = HttpEntity(emptyMap<String, String>());
        val response: ResponseEntity<ByteArray> = restTemplate.exchange(sourceUrl, HttpMethod.GET, httpEntity, String);
        if (!response.statusCode.is2xxSuccessful) {
            logger.error("Failed to update pumpkin: source return status code {}", response.statusCode);
            throw RequestFailedException(response.statusCodeValue);
        }
        val headers: List<Http.THeader> = response.headers
            .filter { !headerBlacklist.contains(it.key.toLowerCase()) }
            .flatMap {
                val header = it.key;
                it.value.map {
                    Http.THeader.newBuilder()
                        .setName(header)
                        .setValue(it)
                        .build()
                }
            }
        return Http.THttpResponse.newBuilder()
            .setStatusCode(response.statusCodeValue)
            .addAllHeaders(headers)
            .setContent(ByteString.copyFrom(response.body))
            .build()
    }

    @Scheduled(
        initialDelayString = "\${pumpkin.update_interval.ms}",
        fixedRateString = "\${pumpkin.update_interval.ms}"
    )
    fun update() {
        try {
            val newResponse = querySource();
            this.cachedResponse = newResponse;
            this.lastUpdatedAt = Instant.now()
            this.successCounter.inc()
        } catch (e: Exception) {
            logger.error("Failed to update pumpkin cache", e);
            this.errorCounter.inc()
        }
    }

    override fun get(): Http.THttpResponse {
        return cachedResponse
    }

    override fun age(): Duration {
        return Duration.between(lastUpdatedAt, Instant.now())
    }

}

@RestController
@RequestMapping("/pumpkin")
class PumpkinController(
    private val pumpkin: Pumpkin
) {

    @RequestMapping("")
    fun get(request: HttpServletRequest, response: HttpServletResponse) {
        val pumpkinProto = pumpkin.get()
        response.status = pumpkinProto.statusCode
        pumpkinProto.headersList.forEach { response.setHeader(it.name, it.value) }
        response.outputStream.write(pumpkinProto.content.toByteArray())
        response.outputStream.flush()
    }

}
