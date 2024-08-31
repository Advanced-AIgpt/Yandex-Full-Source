package ru.yandex.alice.paskills.common.ydb.listener

import com.yandex.ydb.core.Status
import com.yandex.ydb.core.StatusCode
import ru.yandex.monlib.metrics.histogram.Histograms
import ru.yandex.monlib.metrics.labels.Labels
import ru.yandex.monlib.metrics.primitives.Histogram
import ru.yandex.monlib.metrics.primitives.Rate
import ru.yandex.monlib.metrics.registry.MetricRegistry
import java.time.Duration
import java.util.concurrent.ConcurrentHashMap
import java.util.function.Supplier

class MetricsYDBQueryListener(
    private val metricRegistry: MetricRegistry,
    private val logWarnDuration: Duration = Duration.ofMillis(300)
) : YDBQueryListener {

    private val collector = Supplier { Histograms.exponential(22, 1.5, 1.0) }

    private val sessionCreationInvocation = ConcurrentHashMap<String, Rate>()
    private val sessionCreationTimes = ConcurrentHashMap<String, Histogram>()

    private val sessionCreationLongInvocations = ConcurrentHashMap<String, Rate>()
    private val sessionCreationLongTimes = ConcurrentHashMap<String, Histogram>()

    private val retry = ConcurrentHashMap<String, Rate>()

    private val execInvocations = ConcurrentHashMap<String, Rate>()
    private val execErrors = ConcurrentHashMap<String, Rate>()
    private val execTimes = ConcurrentHashMap<String, Histogram>()
    private val execLongInvocations = ConcurrentHashMap<String, Rate>()

    private val failures = ConcurrentHashMap<String, Rate>()

    override fun onQueryStarted(queryName: String, duration: Duration) {
        sessionCreationInvocation.computeIfAbsent(queryName) { query ->
            metricRegistry.rate("ydb.query.session_creation.invocations", Labels.of("query", query))
        }.inc()
        sessionCreationTimes.computeIfAbsent(queryName) { query ->
            metricRegistry.histogramRate("ydb.query.session_creation.times", Labels.of("query", query), collector)
        }.record(duration.toMillis())

        if (duration > logWarnDuration) {
            sessionCreationLongInvocations.computeIfAbsent(queryName) { query ->
                metricRegistry.rate("ydb.query.session_creation_long.invocations", Labels.of("query", query))
            }.inc()
            sessionCreationLongTimes.computeIfAbsent(queryName) { query ->
                metricRegistry.histogramRate(
                    "ydb.query.session_creation_long.times",
                    Labels.of("query", query),
                    collector
                )
            }.record(duration.toMillis())
        }
    }

    override fun onRetry(queryName: String, retryNumber: Int) {
        retry.computeIfAbsent(queryName) { query ->
            metricRegistry.rate("ydb.query.retry", Labels.of("query", query))
        }.inc()
    }

    override fun onQuerySuccess(queryName: String, duration: Duration) {
        countRequest(queryName, duration)
    }

    override fun onQueryFailed(queryName: String, duration: Duration, status: Status?) {

        countRequest(queryName, duration)

        // if parent grpc request was canceled (in apphost it happens on soft retry) don't treat YDB cancellation is an error
        if (status?.code != StatusCode.CLIENT_CANCELLED) {
            execErrors.computeIfAbsent(queryName) { query ->
                metricRegistry.rate("ydb.query.exec.errors", Labels.of("query", query))
            }.inc()
        }
        failures.computeIfAbsent(status?.code?.toString()?.lowercase() ?: "unknown_status") { statusCode ->
            metricRegistry.rate(
                "ydb.query.fails",
                Labels.of("cause", statusCode)
            )
        }.inc()
    }

    private fun countRequest(queryName: String, duration: Duration) {
        execInvocations.computeIfAbsent(queryName) { query ->
            metricRegistry.rate("ydb.query.exec.invocations", Labels.of("query", query))
        }.inc()

        execTimes.computeIfAbsent(queryName) { query ->
            metricRegistry.histogramRate("ydb.query.exec.times", Labels.of("query", query), collector)
        }.record(duration.toMillis())

        if (duration > logWarnDuration) {
            execLongInvocations.computeIfAbsent(queryName) { query ->
                metricRegistry.rate("ydb.query.exec_long.invocations", Labels.of("query", query))
            }.inc()
        }
    }
}
