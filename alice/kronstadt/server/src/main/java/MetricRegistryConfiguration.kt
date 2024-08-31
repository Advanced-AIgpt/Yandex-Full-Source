package ru.yandex.alice.kronstadt.server

import org.springframework.boot.autoconfigure.condition.ConditionalOnMissingBean
import org.springframework.context.annotation.Bean
import org.springframework.context.annotation.Configuration
import ru.yandex.monlib.metrics.JvmGc
import ru.yandex.monlib.metrics.JvmMemory
import ru.yandex.monlib.metrics.JvmRuntime
import ru.yandex.monlib.metrics.JvmThreads
import ru.yandex.monlib.metrics.registry.MetricRegistry

@Configuration
open class MetricRegistryConfiguration {

    @ConditionalOnMissingBean(MetricRegistry::class)
    @Bean
    open fun metricRegistry(): MetricRegistry = MetricRegistry()
        .also { registry ->
            JvmGc.addMetrics(registry)
            JvmMemory.addMetrics(registry)
            JvmRuntime.addMetrics(registry)
            JvmThreads.addMetrics(registry)
        }
}
