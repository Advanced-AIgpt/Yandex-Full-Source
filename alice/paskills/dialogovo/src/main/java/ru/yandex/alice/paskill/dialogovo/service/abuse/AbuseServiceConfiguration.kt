package ru.yandex.alice.paskill.dialogovo.service.abuse

import org.springframework.beans.factory.annotation.Qualifier
import org.springframework.beans.factory.annotation.Value
import org.springframework.context.annotation.Bean
import org.springframework.context.annotation.Configuration
import org.springframework.core.env.Environment
import ru.yandex.alice.paskill.dialogovo.solomon.SolomonUtils
import ru.yandex.alice.paskills.common.resttemplate.factory.RestTemplateClientFactory
import ru.yandex.monlib.metrics.registry.MetricRegistry
import java.time.Duration

@Configuration
internal open class AbuseServiceConfiguration {
    @Bean
    open fun abuseService(
        restTemplateClientFactory: RestTemplateClientFactory,
        @Qualifier("internalMetricRegistry") metricRegistry: MetricRegistry,
        @Value("\${abuseConfig.timeout}") timeout: Int,
        @Value("\${abuseConfig.connectTimeout}") connectTimeout: Int,
        @Value("\${abuseConfig.enabled}") enabled: Boolean,
        @Value("\${abuseConfig.url}") url: String,
        @Value("\${abuseConfig.cacheSize}") cacheSize: Long,
        env: Environment
    ): AbuseService {

        val restTemplate = restTemplateClientFactory
            .serviceWebClientWithRetry(
                "abuse",
                Duration.ofMillis(250),
                2, true,
                Duration.ofMillis(connectTimeout.toLong()),
                Duration.ofMillis(timeout.toLong()),
                100
            )

        val abuseService = AbuseServiceImpl(restTemplate, url, cacheSize)
        SolomonUtils.measureCacheStats(metricRegistry, "abuse_service") { abuseService.cacheStats }
        return abuseService
    }
}
