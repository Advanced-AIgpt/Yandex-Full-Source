package ru.yandex.alice.paskill.dialogovo.service.recommender

import org.springframework.beans.factory.annotation.Qualifier
import org.springframework.beans.factory.annotation.Value
import org.springframework.context.annotation.Bean
import org.springframework.context.annotation.Configuration
import ru.yandex.alice.paskill.dialogovo.config.RecommenderConfig
import ru.yandex.alice.paskills.common.resttemplate.factory.RestTemplateClientFactory
import ru.yandex.monlib.metrics.registry.MetricRegistry
import java.time.Duration

@Configuration
internal open class RecommenderConfiguration {
    @Bean
    open fun recommenderServiceImpl(
        recommenderConfig: RecommenderConfig,
        restTemplateClientFactory: RestTemplateClientFactory,
        @Qualifier("internalMetricRegistry") metricRegistry: MetricRegistry,
        @Value("\${recommender-config.url}") recommenderUrl: String

    ): RecommenderServiceImpl {
        return RecommenderServiceImpl(
            restTemplateClientFactory
                .serviceWebClientWithRetry(
                    "recommender",
                    Duration.ofMillis(100),
                    1, true,
                    Duration.ofMillis(recommenderConfig.connectTimeout.toLong()),
                    Duration.ofMillis(recommenderConfig.timeout.toLong()),
                    100
                ),
            metricRegistry,
            recommenderUrl
        )
    }
}
