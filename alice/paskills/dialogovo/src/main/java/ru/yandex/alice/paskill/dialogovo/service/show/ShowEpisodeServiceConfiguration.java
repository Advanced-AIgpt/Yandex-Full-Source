package ru.yandex.alice.paskill.dialogovo.service.show;

import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.boot.autoconfigure.condition.ConditionalOnProperty;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.alice.paskill.dialogovo.config.ShowConfig;
import ru.yandex.alice.paskill.dialogovo.processor.SkillRequestProcessor;
import ru.yandex.alice.paskill.dialogovo.providers.skill.ShowProvider;
import ru.yandex.alice.paskill.dialogovo.utils.executor.DialogovoInstrumentedExecutorService;
import ru.yandex.alice.paskill.dialogovo.utils.executor.ExecutorsFactory;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

@Configuration
@ConditionalOnProperty(name = "scheduler.controllers.enable", havingValue = "true")
public class ShowEpisodeServiceConfiguration {
    @Bean
    @SuppressWarnings("ParameterNumber")
    public ShowService showService(
            ShowProvider showProvider,
            SkillRequestProcessor skillRequestProcessor,
            ShowEpisodeStoreDao store,
            @Qualifier("showEpisodeFetchServiceExecutor") DialogovoInstrumentedExecutorService fetchService,
            ShowConfig showConfig,
            @Qualifier("internalMetricRegistry") MetricRegistry metricRegistry,
            MorningShowEpisodeDao morningShowEpisodeDao
    ) {
        return new ShowServiceImpl(
                showProvider, skillRequestProcessor, store,
                fetchService, showConfig, metricRegistry, morningShowEpisodeDao
        );
    }

    @Configuration
    static class ShowEpisodeServiceExecutorConfig {
        @Bean(value = "showEpisodeFetchServiceExecutor", destroyMethod = "shutdownNow")
        @Qualifier("showEpisodeFetchServiceExecutor")
        public DialogovoInstrumentedExecutorService showEpisodeFetchServiceExecutor(ExecutorsFactory executorsFactory) {
            return executorsFactory.cachedBoundedThreadPool(2, 100, 100, "show-service.fetch-episodes");
        }
    }
}
