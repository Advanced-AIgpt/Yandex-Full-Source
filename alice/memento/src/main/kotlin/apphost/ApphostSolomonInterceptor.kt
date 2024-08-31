package ru.yandex.alice.memento.apphost

import org.apache.logging.log4j.LogManager
import org.springframework.core.annotation.Order
import org.springframework.stereotype.Component
import ru.yandex.alice.paskills.common.apphost.spring.ApphostRequestHandlingContext
import ru.yandex.alice.paskills.common.apphost.spring.ApphostRequestInterceptor
import ru.yandex.alice.paskills.common.solomon.utils.RequestSensors
import ru.yandex.monlib.metrics.histogram.Histograms
import ru.yandex.monlib.metrics.labels.Labels
import ru.yandex.monlib.metrics.registry.MetricRegistry
import ru.yandex.web.apphost.api.request.RequestContext
import java.time.Duration
import java.time.Instant
import java.util.concurrent.CancellationException
import java.util.concurrent.CompletionException
import java.util.concurrent.ConcurrentHashMap
import java.util.concurrent.ConcurrentMap
import java.util.function.Supplier

@Component
@Order(2)
class ApphostSolomonInterceptor(private val metricRegistry: MetricRegistry) : ApphostRequestInterceptor {
    private val startTimeAttribute = ApphostSolomonInterceptor::class.java.name + ".RequestStartTime"
    private val logger = LogManager.getLogger()

    private val allRequestRate = metricRegistry.rate("apphost.total_requests_rate")
    private val requestCounterCache: ConcurrentMap<String, RequestSensors> = ConcurrentHashMap()
    private val hist = Supplier { Histograms.exponential(22, 2.0) }

    override fun preHandle(handlingContext: ApphostRequestHandlingContext, request: RequestContext): Boolean {
        handlingContext.setAttribute(startTimeAttribute, Instant.now())
        return true
    }

    override fun afterCompletion(
        handlingContext: ApphostRequestHandlingContext,
        request: RequestContext,
        ex: Exception?
    ) {

        try {
            allRequestRate.inc()
            val startTimeInstant = handlingContext.getAttribute(startTimeAttribute) as Instant?
            if (startTimeInstant != null) {

                val path: String = handlingContext.path
                val sensors: RequestSensors = requestCounterCache.computeIfAbsent(path) { key ->
                    RequestSensors.withLabels(
                        metricRegistry, Labels.of("path", key), "apphost.request.", hist
                    )
                }

                sensors.requestRate.inc()
                if (ex != null && (ex !is CompletionException || ex.cause !is CancellationException)) {
                    sensors.failureRate.inc()
                }
                val duration = Duration.between(startTimeInstant, Instant.now())
                sensors.requestTimings.record(duration.toMillis())
            }
        } catch (e: java.lang.Exception) {
            logger.error("ApphostSolomonInterceptor error", e)
        }
    }
}
