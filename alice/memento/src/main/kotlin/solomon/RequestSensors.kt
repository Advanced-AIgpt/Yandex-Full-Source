package ru.yandex.alice.memento.solomon

import ru.yandex.monlib.metrics.histogram.HistogramCollector
import ru.yandex.monlib.metrics.histogram.Histograms
import ru.yandex.monlib.metrics.labels.Labels
import ru.yandex.monlib.metrics.primitives.Counter
import ru.yandex.monlib.metrics.primitives.Histogram
import ru.yandex.monlib.metrics.primitives.Rate
import ru.yandex.monlib.metrics.registry.MetricRegistry
import java.util.function.Supplier

data class RequestSensors(
    val requestRate: Rate,
    val requestCounter: Counter,
    val failureRate: Rate,
    val requestTimings: Histogram,
    val requestBodySize: Histogram,
    val responseBodySize: Histogram,
) {
    companion object {
        @JvmStatic
        @JvmOverloads
        fun withLabels(
            registry: MetricRegistry,
            labels: Labels,
            prefix: String? = null,
            histogramCollectorSupplier: Supplier<HistogramCollector> =
                Supplier { Histograms.exponential(19, 1.5, 10.3 / 1.5) },
            requestBodySizeHistogram: Supplier<HistogramCollector> =
                Supplier { Histograms.exponential(20, 2.0, 10.0) },
            responseBodySizeHistogram: Supplier<HistogramCollector> =
                Supplier { Histograms.exponential(20, 2.0, 10.0) }
        ): RequestSensors {
            val p = prefix ?: ""
            return RequestSensors(
                registry.rate(p + "requests_rate", labels),
                registry.counter(p + "requests_counter", labels),
                registry.rate(p + "requests_failure_rate", labels), // scale to have a bucket on 3 seconds
                registry.histogramRate(p + "requests_duration", labels, histogramCollectorSupplier),
                registry.histogramRate(p + "request_body_size", labels, requestBodySizeHistogram),
                registry.histogramRate(p + "response_body_size", labels, responseBodySizeHistogram)
            )
        }
    }
}
