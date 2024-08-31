package ru.yandex.alice.paskill.dialogovo.service.memento;

import java.time.Duration;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.alice.paskill.dialogovo.config.MementoConfig;
import ru.yandex.alice.paskills.common.resttemplate.factory.RestTemplateClientFactory;
import ru.yandex.monlib.metrics.registry.MetricRegistry;
import ru.yandex.passport.tvmauth.TvmClient;

@Configuration
class MementoConfiguration {

    @Bean
    MementoServiceImpl mementoServiceImpl(MementoConfig mementoConfig,
                                          RestTemplateClientFactory restTemplateClientFactory,
                                          MetricRegistry metricRegistry,
                                          TvmClient tvmClient) {
        return new MementoServiceImpl(
                restTemplateClientFactory
                        .serviceWebClientWithRetry(
                                "memento",
                                Duration.ofMillis(100),
                                1, true,
                                Duration.ofMillis(mementoConfig.getConnectTimeout()),
                                Duration.ofMillis(mementoConfig.getTimeout()),
                                100),
                mementoConfig,
                metricRegistry,
                tvmClient);
    }
}
