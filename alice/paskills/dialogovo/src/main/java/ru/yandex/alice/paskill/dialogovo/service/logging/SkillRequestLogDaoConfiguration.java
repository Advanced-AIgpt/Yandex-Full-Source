package ru.yandex.alice.paskill.dialogovo.service.logging;

import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.alice.paskill.dialogovo.config.YdbConfig;
import ru.yandex.alice.paskill.dialogovo.utils.executor.DialogovoInstrumentedExecutorService;
import ru.yandex.alice.paskill.dialogovo.utils.executor.ExecutorsFactory;
import ru.yandex.alice.paskill.dialogovo.ydb.YdbClient;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

@Configuration
public class SkillRequestLogDaoConfiguration {

    @Bean
    public SkillRequestLogPersistent getSkillRequestLogDao(
            YdbClient ydbClient,
            YdbConfig ydbConfig,
            @Qualifier("skillRequestLogServiceExecutor") DialogovoInstrumentedExecutorService executor,
            @Qualifier("internalMetricRegistry") MetricRegistry metricRegistry
    ) {

        YdbSkillRequestPersistent dao = new YdbSkillRequestPersistent(ydbClient, executor, metricRegistry);
        if (ydbConfig.isWarmUpOnStart()) {
            dao.prepareQueries();
        }
        return dao;
    }

    @Bean(value = "skillRequestLogServiceExecutor", destroyMethod = "shutdownNow")
    public DialogovoInstrumentedExecutorService skillRequestLogServiceExecutor(ExecutorsFactory executorsFactory) {
        return executorsFactory.cachedBoundedThreadPool(2, 100, 100, "skill-request-log");
    }
}
