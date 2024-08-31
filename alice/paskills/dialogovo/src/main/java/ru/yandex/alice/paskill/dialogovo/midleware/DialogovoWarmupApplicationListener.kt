package ru.yandex.alice.paskill.dialogovo.midleware

import com.google.common.base.Stopwatch
import com.google.protobuf.Message
import com.google.protobuf.util.JsonFormat
import org.apache.logging.log4j.LogManager
import org.springframework.beans.factory.annotation.Qualifier
import org.springframework.beans.factory.annotation.Value
import org.springframework.boot.context.event.ApplicationReadyEvent
import org.springframework.boot.web.client.RestTemplateBuilder
import org.springframework.context.ApplicationListener
import org.springframework.core.io.support.PathMatchingResourcePatternResolver
import org.springframework.http.HttpEntity
import org.springframework.http.HttpHeaders
import org.springframework.stereotype.Component
import org.springframework.web.client.postForEntity
import ru.yandex.alice.megamind.protos.scenarios.RequestProto
import ru.yandex.alice.megamind.protos.scenarios.ResponseProto
import ru.yandex.alice.paskills.common.tvm.spring.handler.SecurityHeaders
import ru.yandex.passport.tvmauth.TvmClient
import java.util.concurrent.TimeUnit

@Component
class DialogovoWarmupApplicationListener(
    private val tvmClient: TvmClient,
    private val warmupGuard: ApplicationStartupWarmupFilter,
    private val protoJsonParser: JsonFormat.Parser,
    @Qualifier("baseRestTemplateBuilder") clientBuilder: RestTemplateBuilder,
) : ApplicationListener<ApplicationReadyEvent> {

    private val logger = LogManager.getLogger()
    private val client = clientBuilder.build()

    @Value("\${warmup.requests:0}")
    private val warmupRequests: Int = 0

    override fun onApplicationEvent(event: ApplicationReadyEvent) {
        if (warmupRequests == 0) {
            logger.info("Skipping warmup requests")
            warmupGuard.ready.set(true)
            return
        }
        logger.info("Dialogovo on start warmup requests started")
        val swTotal = Stopwatch.createStarted()

        val headers = HttpHeaders()

        headers[HttpHeaders.ACCEPT] = "application/protobuf"
        headers[HttpHeaders.CONTENT_TYPE] = "application/protobuf"
        headers[SecurityHeaders.SERVICE_TICKET_HEADER] = tvmClient.getServiceTicketFor("self")
        headers[ApplicationStartupWarmupFilter.WARMUP_HEADER] = "true"

        val port = event.applicationContext.environment.getProperty("local.server.port")

        logger.info("Making $warmupRequests run warmup requests")

        val runRequestBuilder = RequestProto.TScenarioRunRequest.newBuilder()
        PathMatchingResourcePatternResolver()
            .getResource("classpath:warmup_run_request.json")
            .inputStream
            .reader().use {
                protoJsonParser.merge(
                    it,
                    runRequestBuilder
                )
            }

        val runBody = runRequestBuilder.build()
        val runHttpEntity = HttpEntity<Message>(runBody, headers)
        val swRunTotal = Stopwatch.createStarted()
        for (i in 1..warmupRequests) {
            val sw = Stopwatch.createStarted()
            try {
                client.postForEntity<ResponseProto.TScenarioRunResponse>(
                    "http://localhost:${port}/megamind/run",
                    runHttpEntity
                )
                logger.info("$i warmup run request done in ${sw.elapsed(TimeUnit.MILLISECONDS)}ms")
            } catch (e: Exception) {
                logger.error("$i warmup run request failed", e)
            }
        }

        logger.info("All warmup run requests done in ${swRunTotal.elapsed(TimeUnit.MILLISECONDS)}ms")

        logger.info("Making $warmupRequests apply warmup requests")

        val applyRequestBuilder = RequestProto.TScenarioApplyRequest.newBuilder()
        PathMatchingResourcePatternResolver()
            .getResource("classpath:warmup_apply_request.json")
            .inputStream
            .reader().use {
                protoJsonParser.merge(
                    it,
                    applyRequestBuilder
                )
            }
        val applyBody = applyRequestBuilder.build()
        val applyHttpEntity = HttpEntity<Message>(applyBody, headers)
        val swApplyTotal = Stopwatch.createStarted()
        for (i in 1..warmupRequests) {
            val sw = Stopwatch.createStarted()
            try {
                client.postForEntity<ResponseProto.TScenarioApplyResponse>(
                    "http://localhost:${port}/megamind/apply",
                    applyHttpEntity
                )
                logger.info("$i warmup apply request done in ${sw.elapsed(TimeUnit.MILLISECONDS)}ms")
            } catch (e: Exception) {
                logger.error("$i warmup apply request failed", e)
            }
        }

        logger.info("All warmup apply requests done in ${swApplyTotal.elapsed(TimeUnit.MILLISECONDS)}ms")


        warmupGuard.ready.set(true)

        logger.info("All warmup requests done in ${swTotal.elapsed(TimeUnit.MILLISECONDS)}ms")
    }
}
