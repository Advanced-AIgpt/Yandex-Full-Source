package ru.yandex.alice.kronstadt.scenarios.tv_channels.indexer

import com.fasterxml.jackson.databind.ObjectMapper
import org.springframework.beans.factory.annotation.Value
import org.springframework.boot.autoconfigure.condition.ConditionalOnProperty
import org.springframework.boot.web.client.RestTemplateBuilder
import org.springframework.context.annotation.Bean
import org.springframework.context.annotation.Configuration
import org.springframework.http.client.HttpComponentsClientHttpRequestFactory
import ru.yandex.alice.paskills.common.executor.InstrumentedExecutorFactory
import ru.yandex.alice.paskills.common.logging.protoseq.ClientSetraceInterceptor
import ru.yandex.alice.paskills.common.resttemplate.factory.client.HttpClientFactory
import ru.yandex.monlib.metrics.registry.MetricRegistry
import java.time.Duration
import java.util.Optional

@Configuration
open class SaasIndexerClientConfiguration {
    @Value("\${tv-channel-indexer.saas-indexer.connect_timeout}")
    private val connectTimeout: Long = 30

    @Value("\${tv-channel-indexer.saas-indexer.timeout}")
    private val readTimeout: Long = 4000

    @Value("\${tv-channel-indexer.saas-indexer.max_connections}")
    private val maxConnections: Int = 150

    @Value("\${tv-channel-indexer.saas-indexer.url}")
    private lateinit var url: String

    @Bean
    @ConditionalOnProperty("tests.it2", havingValue = "false", matchIfMissing = true)
    open fun saasIndexerClientImpl(
        objectMapper: ObjectMapper,
        httpClientFactory: HttpClientFactory,
        baseBuilder: RestTemplateBuilder,
        metricRegistry: MetricRegistry,
    ): SaasIndexerClient {
        val client = httpClientFactory.builder()
            .connectTimeout(Duration.ofMillis(this.connectTimeout)) // as we use only POST requests with `false` flag we can retry connection exceptions
            // which occur even before we transfer request body to the target
            .retryOnIO(2, false)
            .instrumented("http.out", "saas_daemon", false, Optional.empty())
            .maxConnectionCount(maxConnections)
            .maxConnectionsPerRoute(maxConnections)
            .disableRedirects()
            .disableCookieManagement()
            .build()

        val requestFactory = HttpComponentsClientHttpRequestFactory(client)

        val restTemplateBuilder = baseBuilder
            .requestFactory { requestFactory }
            .additionalInterceptors(ClientSetraceInterceptor("saas_daemon", false))
            .setReadTimeout(Duration.ofMillis(this.readTimeout.toLong()))
            .setConnectTimeout(Duration.ofMillis(this.connectTimeout.toLong()))
            .build()

        val executor = InstrumentedExecutorFactory.fixedThreadPool(
            name = "saas-indexer",
            threads = maxConnections,
            queueCapacity = maxConnections * 5,
            metricRegistry = metricRegistry,
        )

        return SaasIndexerClientImpl(
            restTemplate = restTemplateBuilder,
            objectMapper = objectMapper,
            url = url,
            executorService = executor,
        )
    }

    @Bean
    @ConditionalOnProperty("tests.it2", matchIfMissing = false)
    open fun noopIndexerClient(): SaasIndexerClient {
        return NoopSaasIndexerClient()
    }
}
