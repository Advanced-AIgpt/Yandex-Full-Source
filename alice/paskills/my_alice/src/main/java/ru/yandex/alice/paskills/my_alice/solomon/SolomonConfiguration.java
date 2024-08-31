package ru.yandex.alice.paskills.my_alice.solomon;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.monlib.metrics.JvmGc;
import ru.yandex.monlib.metrics.JvmMemory;
import ru.yandex.monlib.metrics.JvmRuntime;
import ru.yandex.monlib.metrics.JvmThreads;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

@Configuration
public class SolomonConfiguration {

    @Bean
    public MetricRegistry metricRegistry() {
        var registry = new MetricRegistry();
        JvmGc.addMetrics(registry);
        JvmMemory.addMetrics(registry);
        JvmRuntime.addMetrics(registry);
        JvmThreads.addMetrics(registry);
        return registry;
    }

}
