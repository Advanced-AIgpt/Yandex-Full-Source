package ru.yandex.alice.memento.storage

import com.google.common.base.Stopwatch
import com.google.protobuf.Message
import org.apache.logging.log4j.LogManager
import org.springframework.beans.factory.annotation.Value
import org.springframework.boot.ApplicationArguments
import org.springframework.boot.ApplicationRunner
import org.springframework.core.env.Environment
import org.springframework.http.HttpEntity
import org.springframework.http.HttpHeaders
import org.springframework.stereotype.Component
import org.springframework.web.client.RestTemplate
import org.springframework.web.client.postForEntity
import ru.yandex.alice.memento.apphost.ApphostWarmupInterceptor
import ru.yandex.alice.memento.controller.ApplicationStartupWarmupFilter
import ru.yandex.alice.memento.controller.CustomProtobufHttpMessageConverter
import ru.yandex.alice.memento.proto.MementoApiProto
import ru.yandex.alice.paskills.common.tvm.spring.handler.SecurityHeaders
import ru.yandex.passport.tvmauth.TvmClient
import ru.yandex.web.apphost.api.AppHostService
import ru.yandex.web.apphost.grpc.Client
import java.util.concurrent.TimeUnit

@Component
internal class WarmupApplicationRunner(
    private val tvmClient: TvmClient,
    private val protoConverter: CustomProtobufHttpMessageConverter,
    private val warmupGuard: ApplicationStartupWarmupFilter,
    private val apphostWarmupInterceptor: ApphostWarmupInterceptor,
    private val env: Environment,
    private val appHostService: AppHostService,
) : ApplicationRunner {

    private val logger = LogManager.getLogger()

    @Value("\${warmup.requests:50}")
    private val warmupRequests: Int = 0

    override fun run(args: ApplicationArguments) {
        warmupWithRequests()
        // wait a bit to cool down JVM.
        Thread.sleep(5000)
        warmupGuard.ready.set(true)
        apphostWarmupInterceptor.ready.set(true)
    }

    private fun warmupWithRequests() {
        logger.info("Start warmup requests")
        val client = RestTemplate()
        client.messageConverters.add(protoConverter)

        val body = MementoApiProto.TReqGetAllObjects.newBuilder()
            .addSurfaceId("tmp_surf")
            .setCurrentSurfaceId("tmp_surf")
            .build()
        val headers = HttpHeaders()

        headers[HttpHeaders.ACCEPT] = "application/protobuf"
        headers[HttpHeaders.CONTENT_TYPE] = "application/protobuf"
        headers[SecurityHeaders.SERVICE_TICKET_HEADER] = tvmClient.getServiceTicketFor("self")
        headers[ApplicationStartupWarmupFilter.WARMUP_HEADER] = "true"

        val httpEntity = HttpEntity<Message>(body, headers)
        val port = env.getProperty("local.server.port")
        logger.info("Making ${warmupRequests} warmup requests")
        val swTotal = Stopwatch.createStarted()
        for (i in 0..warmupRequests) {
            val sw = Stopwatch.createStarted()
            try {
                client.postForEntity<MementoApiProto.TRespGetAllObjects>(
                    "http://localhost:${port}/get_all_objects",
                    httpEntity
                )
                logger.info("$i warmup request done in ${sw.elapsed(TimeUnit.MILLISECONDS)}ms")
            } catch (e: Exception) {
                logger.error("$i warmup request failed", e)
            }
        }

        logger.info("Http warmup finished in ${swTotal.elapsed(TimeUnit.MILLISECONDS)}ms")

        swTotal.reset().start()
        logger.info("Starting apphost warmup")
        Client("localhost", appHostService.port).use { apphostClient ->

            for (i in 0..warmupRequests) {
                val sw = Stopwatch.createStarted()
                try {
                    apphostClient.call("/get_all_objects") {
                        addProtobufItem("get_all_objects_request", body)

                        addFlag(ApphostWarmupInterceptor.WARMUP_FLAG)

                    }
                    logger.info("$i warmup request done in ${sw.elapsed(TimeUnit.MILLISECONDS)}ms")
                } catch (e: Exception) {
                    logger.error("$i warmup request failed", e)
                }
            }
        }
        logger.info("Apphost warmup requests done in ${swTotal.elapsed(TimeUnit.MILLISECONDS)}ms")
    }
}
