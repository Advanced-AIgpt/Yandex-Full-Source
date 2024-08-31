package ru.yandex.alice.paskill.dialogovo.service.normalizer;

import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.alice.paskill.dialogovo.solomon.SolomonUtils;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

@Configuration
class NormalizationServiceConfiguration {
    @Bean
    NormalizationService normalizationService(@Qualifier("internalMetricRegistry") MetricRegistry metricRegistry) {
        NormalizationServiceImpl service = new NormalizationServiceImpl();

        SolomonUtils.measureCacheStats(metricRegistry, "normalization_service", service::getCacheStats);

        return service;
    }
}
