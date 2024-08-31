package ru.yandex.alice.memento.controller

import com.google.common.base.Stopwatch
import org.springframework.core.annotation.Order
import org.springframework.stereotype.Component
import ru.yandex.monlib.metrics.histogram.HistogramCollector
import ru.yandex.monlib.metrics.histogram.Histograms
import ru.yandex.monlib.metrics.labels.Labels
import ru.yandex.monlib.metrics.primitives.Histogram
import ru.yandex.monlib.metrics.primitives.Rate
import ru.yandex.monlib.metrics.registry.MetricRegistry
import java.util.concurrent.ConcurrentHashMap
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicBoolean
import javax.servlet.Filter
import javax.servlet.FilterChain
import javax.servlet.ServletRequest
import javax.servlet.ServletResponse
import javax.servlet.http.HttpServletRequest
import javax.servlet.http.HttpServletResponse

@Order(15)
@Component
internal class ApplicationStartupWarmupFilter(private val metricRegistry: MetricRegistry) : Filter {
    internal val ready = AtomicBoolean(false)

    private val hist: () -> HistogramCollector = { Histograms.exponential(19, 1.5, 1.0) }

    private val requestsRate = ConcurrentHashMap<String, Rate>(10)
    private val requests503Rate = ConcurrentHashMap<String, Rate>(10)
    private val requestsTimes = ConcurrentHashMap<String, Histogram>(10)

    override fun doFilter(request: ServletRequest, response: ServletResponse, chain: FilterChain) {
        if (ready.get()) {
            chain.doFilter(request, response)
        } else if (request is HttpServletRequest && response is HttpServletResponse) {
            val sw = Stopwatch.createStarted()

            if (request.serverName == "localhost" ||
                request.getHeader(WARMUP_HEADER) == "true" ||
                request.servletPath.contains("solomon") ||
                request.servletPath.contains("actuator")
            ) {
                chain.doFilter(request, response)
                requestsRate.computeIfAbsent(request.servletPath) {
                    metricRegistry.rate("warmup.warmup_request", Labels.of("path", it))
                }.inc()
            } else {
                response.sendError(503, "service not ready")
                requests503Rate.computeIfAbsent(request.servletPath) {
                    metricRegistry.rate("warmup.response_503", Labels.of("path", it))
                }.inc()

                requestsTimes.computeIfAbsent(request.servletPath) {
                    metricRegistry.histogramRate("warmup.duration_503", Labels.of("path", it), hist)
                }.record(sw.elapsed(TimeUnit.MILLISECONDS))
            }
        } else {
            metricRegistry.rate("warmup.non_http_request").inc()
            chain.doFilter(request, response)
        }
    }

    companion object {
        internal const val WARMUP_HEADER = "X-Ya-Memento-Warmup"
    }
}
