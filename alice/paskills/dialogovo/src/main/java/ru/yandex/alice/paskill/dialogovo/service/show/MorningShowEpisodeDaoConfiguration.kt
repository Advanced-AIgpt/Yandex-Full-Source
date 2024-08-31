package ru.yandex.alice.paskill.dialogovo.service.show

import org.springframework.beans.factory.annotation.Qualifier
import org.springframework.context.annotation.Bean
import org.springframework.context.annotation.Configuration
import ru.yandex.alice.paskill.dialogovo.config.YdbConfig
import ru.yandex.alice.paskill.dialogovo.ydb.YdbClient
import ru.yandex.monlib.metrics.registry.MetricRegistry

@Configuration
internal open class MorningShowEpisodeDaoConfiguration {
    @Bean
    open fun morningShowStateDao(
        ydbClient: YdbClient,
        ydbConfig: YdbConfig,
        @Qualifier("internalMetricRegistry") metricRegistry: MetricRegistry
    ): MorningShowEpisodeDao = MorningShowEpisodeDaoImpl(ydbClient, metricRegistry).apply {
        if (ydbConfig.isWarmUpOnStart()) {
            prepareQueries()
        }
    }
}
