package ru.yandex.alice.paskill.dialogovo.service.wizard;

import java.time.Duration;

import com.fasterxml.jackson.databind.ObjectMapper;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.alice.paskill.dialogovo.config.WizardConfig;
import ru.yandex.alice.paskill.dialogovo.solomon.SolomonUtils;
import ru.yandex.alice.paskill.dialogovo.utils.executor.DialogovoInstrumentedExecutorService;
import ru.yandex.alice.paskill.dialogovo.utils.executor.ExecutorsFactory;
import ru.yandex.alice.paskills.common.resttemplate.factory.RestTemplateClientFactory;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

@Configuration
public class WizardServiceConfiguration {
    static final int RETRY_COUNT = 2;

    @Bean
    public WizardService wizardService(
            WizardConfig wizardConfig,
            RestTemplateClientFactory restTemplateClientFactory,
            @Qualifier("wizardServiceExecutor") DialogovoInstrumentedExecutorService executor,
            ObjectMapper objectMapper,
            @Qualifier("internalMetricRegistry") MetricRegistry metricRegistry,
            @Value("${fillCachesOnStartUp}") boolean fillCachesOnStartUp
    ) {

        WizardServiceImpl wizardService = new WizardServiceImpl(wizardConfig,
                restTemplateClientFactory
                        .serviceWebClientWithRetry(
                                "wizard",
                                Duration.ofMillis(250),
                                RETRY_COUNT, true,
                                Duration.ofMillis(wizardConfig.getConnectTimeout()),
                                Duration.ofMillis(wizardConfig.getTimeout()),
                                100),
                executor,
                objectMapper,
                fillCachesOnStartUp);
        SolomonUtils.measureCacheStats(metricRegistry, "wizard_service", wizardService::getCacheStats);
        return wizardService;
    }

    @Configuration
    static class WizardServiceExecutorConfig {
        @Bean(value = "wizardServiceExecutor", destroyMethod = "shutdownNow")
        public DialogovoInstrumentedExecutorService nerExecutorService(ExecutorsFactory executorsFactory) {
            return executorsFactory.cachedBoundedThreadPool(2, 100, 100, "wizard-service");
        }
    }
}
