package ru.yandex.alice.social.sharing

import org.springframework.context.annotation.Bean
import org.springframework.context.annotation.Configuration
import ru.yandex.monlib.metrics.labels.Labels
import ru.yandex.monlib.metrics.primitives.Counter
import ru.yandex.monlib.metrics.registry.MetricRegistry

fun makeErrorSensor(metricRegistry: MetricRegistry, reason: String): Counter {
    return metricRegistry.counter("error", Labels.of("reason", reason))
}

@Configuration
open class SolomonConfiguration {
    @Bean
    open fun metricRegistry(): MetricRegistry {
        return MetricRegistry.root()
    }
}
