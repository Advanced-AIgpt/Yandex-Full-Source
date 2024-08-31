package ru.yandex.alice.paskill.dialogovo.ydb.listener

import com.google.common.collect.ConcurrentHashMultiset
import com.google.common.collect.Multiset
import com.yandex.ydb.core.Status
import org.springframework.beans.factory.annotation.Qualifier
import org.springframework.stereotype.Component
import ru.yandex.alice.paskills.common.solomon.utils.NamedSensorsRegistry
import ru.yandex.monlib.metrics.labels.Labels
import ru.yandex.monlib.metrics.registry.MetricRegistry
import java.time.Duration

@Component
internal open class MetricsYDBQueryListener(
    @Qualifier("internalMetricRegistry") metricRegistry: MetricRegistry
) : YDBQueryListener {
    private val concurrentQueries: Multiset<String> = ConcurrentHashMultiset.create()
    private val namedMetricRegistry = NamedSensorsRegistry(metricRegistry, "ydb.query")

    override fun onQueryStarted(queryName: String) {
        concurrentQueries.add(queryName)
        countQuery(queryName)
    }

    override fun onRetry(queryName: String, retryNumber: Int) {
        namedMetricRegistry.withLabels(Labels.of("query", queryName))
            .rate("retry")
            .inc()
    }

    override fun onQuerySuccess(queryName: String, duration: Duration) {
        namedMetricRegistry.withLabels(Labels.of("query", queryName))
            .instrument("exec")
            .measure(duration.toMillis(), true)
        countQuery(queryName)
    }

    override fun onQueryFailed(queryName: String, duration: Duration, status: Status?) {
        namedMetricRegistry.withLabels(Labels.of("query", queryName))
            .instrument("exec")
            .measure(duration.toMillis(), false)

        if (status != null) {
            namedMetricRegistry.withLabels(Labels.of("cause", status.code.toString().lowercase()))
                .rate("fails")
                .inc()
        }
        countQuery(queryName)
    }

    private fun countQuery(queryInfo: String) {
        namedMetricRegistry.withLabels(Labels.of("query", "all"))
            .gauge("count")
            .set(concurrentQueries.size.toLong())

        namedMetricRegistry.withLabels(Labels.of("query", queryInfo))
            .gauge("count")
            .set(concurrentQueries.count(queryInfo).toLong())
    }
}
