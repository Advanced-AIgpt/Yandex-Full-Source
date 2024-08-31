package ru.yandex.alice.paskills.my_alice.pumpkin

import org.apache.logging.log4j.LogManager
import org.springframework.beans.factory.annotation.Value
import org.springframework.context.annotation.Bean
import org.springframework.context.annotation.Configuration
import org.springframework.stereotype.Component
import ru.yandex.web.apphost.api.AppHostService
import ru.yandex.web.apphost.api.AppHostServiceBuilder
import ru.yandex.web.apphost.api.healthcheck.AppHostHealthCheck
import ru.yandex.web.apphost.api.healthcheck.AppHostHealthCheckResult
import ru.yandex.web.apphost.api.request.RequestContext
import java.util.function.Consumer

typealias ApphostHandler = Consumer<RequestContext>;

@Component
class PumpkinHandler(
    val pumpkin: Pumpkin
): ApphostHandler {

    override fun accept(ctx: RequestContext) {
        ctx.addProtobufItem("proto_http_response", pumpkin.get())
    }

}

@Component
class HealthCheck: AppHostHealthCheck {
    override fun check(): AppHostHealthCheckResult {
        // app is healthy if all components were created
        return AppHostHealthCheckResult.healthy()
    }
}

@Configuration
open class ApphostConfiguration {

    @Bean(destroyMethod = "stop")
    open fun appHostService(
        @Value("\${apphost.port}") port: Int,
        pumpkinHandler: PumpkinHandler,
        healthCheck: AppHostHealthCheck
    ): AppHostService {
        val appHostService = AppHostServiceBuilder
            .forPort(port)
            .withHealthCheck(healthCheck)
            .withPathHandler("/", pumpkinHandler)
            .build()
        logger.info("Staring apphost service at port {}", port)
        appHostService.start()
        return appHostService
    }

    companion object {
        private val logger = LogManager.getLogger()
    }

}
