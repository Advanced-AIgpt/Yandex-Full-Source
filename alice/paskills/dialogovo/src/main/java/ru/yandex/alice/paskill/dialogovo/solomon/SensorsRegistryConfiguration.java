package ru.yandex.alice.paskill.dialogovo.solomon;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Primary;

import ru.yandex.monlib.metrics.JvmGc;
import ru.yandex.monlib.metrics.JvmMemory;
import ru.yandex.monlib.metrics.JvmRuntime;
import ru.yandex.monlib.metrics.JvmThreads;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

@Configuration
public class SensorsRegistryConfiguration {
    @Bean("internalMetricRegistry")
    @Primary
    public MetricRegistry internalMetricRegistry() {
        var registry = new MetricRegistry();

        JvmGc.addMetrics(registry);
        JvmMemory.addMetrics(registry);
        JvmRuntime.addMetrics(registry);
        JvmThreads.addMetrics(registry);
        return registry;
    }

    @Bean("externalMetricRegistry")
    public MetricRegistry externalMetricRegistry() {
        return new MetricRegistry();
    }
}
