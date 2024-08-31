package ru.yandex.alice.social.sharing

import NAppHostHttp.Http
import com.fasterxml.jackson.databind.ObjectMapper
import org.apache.logging.log4j.LogManager
import org.apache.logging.log4j.ThreadContext
import org.springframework.beans.factory.annotation.Value
import org.springframework.boot.availability.ApplicationAvailability
import org.springframework.boot.availability.LivenessState
import org.springframework.boot.availability.ReadinessState
import org.springframework.context.annotation.Bean
import org.springframework.context.annotation.Configuration
import org.springframework.stereotype.Component
import ru.yandex.web.apphost.api.AppHostService
import ru.yandex.web.apphost.api.AppHostServiceBuilder
import ru.yandex.web.apphost.api.healthcheck.AppHostHealthCheck
import ru.yandex.web.apphost.api.healthcheck.AppHostHealthCheckResult
import ru.yandex.web.apphost.api.request.RequestContext
import java.util.function.Consumer

fun interface HttpBodyParser<T> {
    fun parse(content: ByteArray): T
}

fun <T> ObjectMapper.jsonBodyParser(clazz: Class<T>): HttpBodyParser<T> {
    return HttpBodyParser { content ->
        readValue(content, clazz)
    }
}

internal data class ApphostParams(
    val reqid: String?,
)

private class HttpError(
    val statusCode: Int,
    val body: String,
): Exception("Failed to parse http response: statusCode=$statusCode\n$body")

interface ApphostHandler: Consumer<RequestContext> {
    val path: String

    fun handle(context: RequestContext)

    private fun parseApphostParams(context: RequestContext): ApphostParams? {
        val items = context.getRequestItems("app_host_params")
        if (items.isEmpty()) {
            return null
        }
        val item = items[0].getJsonData(ApphostParams::class.java)
        return item
    }

    private fun setupLoggingContext(context: RequestContext) {
        val apphostParams = parseApphostParams(context)
        ThreadContext.put(LoggingUtils.REQUEST_ID, apphostParams?.reqid ?: "EMPTY_REQUEST_ID")
        ThreadContext.put(LoggingUtils.REQUEST_GUID, context.guid ?: "EMPTY_REQUEST_GUID")
        ThreadContext.put(LoggingUtils.RUID, context.ruid.toString())
    }

    override fun accept(context: RequestContext) {
        setupLoggingContext(context)
        logger.info("Handing apphost request {}", path)
        try {
            handle(context)
        } catch (e: Exception) {
            logger.error("Failed to process apphost request", e)
            throw e
        } finally {
            ThreadContext.clearAll()
        }
    }

    fun <T> parseHttpResponse(
        context: RequestContext,
        item: String,
        parser: HttpBodyParser<T>,
    ): T {
        val proto = context
            .getSingleRequestItem(item)
            .getProtobufData(Http.THttpResponse.getDefaultInstance())
        if (proto.statusCode >= 400) {
            throw HttpError(proto.statusCode, proto.content.toStringUtf8())
        }
        val content = proto.content
        logger.debug("Response content: {}", content.toStringUtf8())
        return parser.parse(content.toByteArray())
    }

    companion object {
        private val logger = LogManager.getLogger()
    }

}

@Component
class HealthCheck(
    private val applicationAvailability: ApplicationAvailability
) : AppHostHealthCheck {

    override fun check(): AppHostHealthCheckResult {
        // app is healthy if all components were created
        return if (
            applicationAvailability.livenessState == LivenessState.CORRECT &&
            applicationAvailability.readinessState == ReadinessState.ACCEPTING_TRAFFIC
        )
            AppHostHealthCheckResult.healthy()
        else
            AppHostHealthCheckResult.unhealthy("Either liveness or readiness check failed")
    }

}

private class ApphostHandlerRedefinitionException(
    val handler: String,
): Exception("Apphost handler $handler was defined multiple times")

@Configuration
open class ApphostConfiguration {

    @Bean(destroyMethod = "stop")
    open fun appHostService(
        @Value("\${apphost.port}") port: Int,
        healthCheck: AppHostHealthCheck,
        handlers: List<ApphostHandler>,
    ): AppHostService {
        val appHostServiceBuilder = AppHostServiceBuilder
            .forPort(port)
            .withHealthCheck(healthCheck)
        val usedHandlers: Set<String> = mutableSetOf()
        for (handler in handlers) {
            logger.info("Adding path handler {}", handler.path)
            if (usedHandlers.contains(handler.path)) {
                throw ApphostHandlerRedefinitionException(handler.path)
            }
            appHostServiceBuilder.withPathHandler(handler.path, handler)
        }
        val appHostService = appHostServiceBuilder.build()
        logger.info("Staring apphost service at port {}", port)
        appHostService.start()
        return appHostService
    }

    companion object {
        private val logger = LogManager.getLogger()
    }

}
