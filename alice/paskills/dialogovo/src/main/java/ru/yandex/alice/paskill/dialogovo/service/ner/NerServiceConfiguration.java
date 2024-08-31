package ru.yandex.alice.paskill.dialogovo.service.ner;

import java.time.Duration;

import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.alice.paskill.dialogovo.config.NerApiConfig;
import ru.yandex.alice.paskill.dialogovo.solomon.SolomonUtils;
import ru.yandex.alice.paskill.dialogovo.utils.executor.DialogovoInstrumentedExecutorService;
import ru.yandex.alice.paskill.dialogovo.utils.executor.ExecutorsFactory;
import ru.yandex.alice.paskills.common.resttemplate.factory.RestTemplateClientFactory;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

@Configuration
class NerServiceConfiguration {

    private static final int RETRY_COUNT = 2;

    @Bean
    NerService nerService(
            NerApiConfig nerConfig,
            RestTemplateClientFactory restTemplateClientFactory,
            @Qualifier("nerServiceExecutor") DialogovoInstrumentedExecutorService executor,
            @Qualifier("internalMetricRegistry") MetricRegistry metricRegistry
    ) {
        NerServiceImpl nerService = new NerServiceImpl(nerConfig, restTemplateClientFactory
                .serviceWebClientWithRetry(
                        "ner",
                        Duration.ofMillis(150),
                        RETRY_COUNT, true,
                        Duration.ofMillis(nerConfig.getConnectTimeout()),
                        Duration.ofMillis(nerConfig.getTimeout()),
                        100), executor,
                RETRY_COUNT);

        SolomonUtils.measureCacheStats(metricRegistry, "ner_service", nerService::getCacheStats);

        return nerService;
    }


    @Bean(value = "nerServiceExecutor")
    public DialogovoInstrumentedExecutorService nerExecutorService(ExecutorsFactory executorsFactory) {
        return executorsFactory.cachedBoundedThreadPool(2, 100, 100, "ner-service");
    }


}
