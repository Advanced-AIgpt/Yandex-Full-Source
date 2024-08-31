package ru.yandex.alice.paskill.dialogovo.service.state

import com.fasterxml.jackson.databind.ObjectMapper
import org.springframework.beans.factory.annotation.Qualifier
import org.springframework.context.annotation.Bean
import org.springframework.context.annotation.Configuration
import ru.yandex.alice.paskill.dialogovo.config.YdbConfig
import ru.yandex.alice.paskill.dialogovo.utils.executor.DialogovoInstrumentedExecutorService
import ru.yandex.alice.paskill.dialogovo.utils.executor.ExecutorsFactory
import ru.yandex.alice.paskill.dialogovo.ydb.YdbClient
import ru.yandex.monlib.metrics.registry.MetricRegistry

@Configuration
internal open class SkillStateConfiguration {
    @Bean
    open fun skillStateDao(
        ydbClient: YdbClient,
        @Qualifier("skillStateServiceExecutor") executor: DialogovoInstrumentedExecutorService,
        objectMapper: ObjectMapper,
        ydbConfig: YdbConfig,
        @Qualifier("internalMetricRegistry") metricRegistry: MetricRegistry
    ): SkillStateDao {
        val stateDao = SkillStateDaoImpl(ydbClient, objectMapper, executor, metricRegistry)
        if (ydbConfig.isWarmUpOnStart()) {
            stateDao.prepareQueries()
        }
        return stateDao
    }

    @Bean(value = ["skillStateServiceExecutor"], destroyMethod = "shutdownNow")
    open fun skillStateServiceExecutor(executorsFactory: ExecutorsFactory): DialogovoInstrumentedExecutorService {
        return executorsFactory.cachedBoundedThreadPool(2, 100, 100, "skill-state")
    }
}
