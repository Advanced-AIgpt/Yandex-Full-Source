package ru.yandex.alice.memento.solomon

import org.apache.logging.log4j.LogManager
import org.springframework.core.annotation.Order
import org.springframework.http.HttpHeaders
import org.springframework.stereotype.Component
import org.springframework.util.StringUtils
import ru.yandex.monlib.metrics.histogram.Histograms
import ru.yandex.monlib.metrics.labels.Labels
import ru.yandex.monlib.metrics.primitives.Rate
import ru.yandex.monlib.metrics.registry.MetricRegistry
import java.util.concurrent.ConcurrentHashMap
import java.util.function.Supplier
import javax.servlet.Filter
import javax.servlet.FilterChain
import javax.servlet.ServletRequest
import javax.servlet.ServletResponse
import javax.servlet.http.HttpServletRequest
import javax.servlet.http.HttpServletResponse

@Order(5)
@Component
internal class SolomonFilter(private val metricRegistry: MetricRegistry) : Filter {
    private val requestCounterCache = ConcurrentHashMap<String, RequestSensors>()
    private val histogramCollectorSupplier = // () -> Histograms.exponential(18, 1.5, 2);
    // similar but explicit collector
    // () -> Histograms.explicit(2, 3, 5, 7, 10, 15, 23, 34, 51, 77, 115, 173, 259, 389, 584, 876, 1314);
        // more granular collector
        Supplier {
            Histograms.explicit(
                2.0,
                3.0,
                5.0,
                7.0,
                10.0,
                15.0,
                20.0,
                25.0,
                30.0,
                35.0,
                40.0,
                45.0,
                50.0,
                60.0,
                70.0,
                80.0,
                90.0,
                100.0,
                115.0,
                130.0,
                145.0,
                160.0,
                175.0,
                259.0,
                389.0,
                584.0,
                876.0,
                1314.0
            )
        }
    private val allRequestRate: Rate = metricRegistry.rate("total_requests_rate")

    init {
        listOf(
            "update_objects",
            "solomon",
            "get_objects",
            "get_all_objects",
            "clear_user_data",
            "actuator_health_readiness"
        ).forEach { path ->
            requestCounterCache.putIfAbsent(
                path,
                RequestSensors.withLabels(
                    metricRegistry,
                    Labels.of("path", path),
                    "http.in.",
                    histogramCollectorSupplier
                )
            )
        }
    }

    override fun doFilter(request: ServletRequest, response: ServletResponse, chain: FilterChain) {
        val start = System.nanoTime()
        allRequestRate.inc()
        val httpRequest = request as HttpServletRequest
        val httpResponse = response as HttpServletResponse
        var ex: Exception? = null
        try {
            chain.doFilter(request, response)
        } catch (e: Exception) {
            ex = e
        } finally {
            try {
                val path = getPath(httpRequest.servletPath)
                val sensors = requestCounterCache
                    .computeIfAbsent(path) { key: String? ->
                        RequestSensors.withLabels(
                            metricRegistry,
                            Labels.of("path", path),
                            "http.in.",
                            histogramCollectorSupplier
                        )
                    }
                sensors.requestRate.inc()
                sensors.requestCounter.inc()
                if (httpRequest.contentLengthLong > 0) {
                    sensors.requestBodySize.record(httpRequest.contentLengthLong)
                }
                if (ex != null || (httpResponse.status >= 400 && httpResponse.status != 503)) {
                    sensors.failureRate.inc()
                } else {
                    val responseCl = httpResponse.getHeader(HttpHeaders.CONTENT_LENGTH)
                    if (responseCl != null) {
                        try {
                            val l = responseCl.toLong()
                            if (l > 0) {
                                sensors.responseBodySize.record(l)
                            }
                        } catch (e: NumberFormatException) {
                            logger.error("Can't parse response content-length header", e)
                        }
                    }
                }
                sensors.requestTimings.record((System.nanoTime() - start) / 1000000)
            } catch (e: Exception) {
                // don't fail
                logger.error(e)
            }
        }
    }

    private fun getPath(servletPath: String): String {
        return StringUtils.trimTrailingCharacter(StringUtils.trimLeadingCharacter(servletPath, '/'), '/')
            .replace("[/\\-.]".toRegex(), "_")
    }

    companion object {
        private val logger = LogManager.getLogger()
    }
}
