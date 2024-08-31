package ru.yandex.alice.kronstadt.scenarios.tv_channels.search

import org.springframework.beans.factory.annotation.Value
import org.springframework.boot.web.client.RestTemplateBuilder
import org.springframework.context.annotation.Bean
import org.springframework.context.annotation.Configuration
import org.springframework.http.client.HttpComponentsClientHttpRequestFactory
import ru.yandex.alice.paskills.common.logging.protoseq.ClientSetraceInterceptor
import ru.yandex.alice.paskills.common.resttemplate.factory.client.HttpClientFactory
import java.time.Duration
import java.util.Optional

@Configuration
open class SaasSearchClientConfiguration {

    @Value("\${tv-channel-indexer.saas-search.url}")
    private lateinit var url: String

    @Value("\${tv-channel-indexer.saas-search.timeout:}")
    private var timeoutMs: Long = 1000

    @Value("\${tv-channel-indexer.saas-search.service-name}")
    private lateinit var serviceName: String

    @Value("\${tv-channel-indexer.saas-search.connect_timeout}")
    private val connectTimeout: Long = 30

    @Value("\${tv-channel-indexer.saas-search.max_connections}")
    private val maxConnections: Int = 50

    @Bean
    open fun saasSearchClient(
        baseBuilder: RestTemplateBuilder,
        httpClientFactory: HttpClientFactory,
    ): SaasSearchClient {

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

        val restTemplate = baseBuilder
            .requestFactory { requestFactory }
            .additionalInterceptors(ClientSetraceInterceptor("saas_daemon", false))
            .setReadTimeout(Duration.ofMillis(this.timeoutMs))
            .setConnectTimeout(Duration.ofMillis(this.connectTimeout))
            .build()

        return SaasSearchClientImpl(restTemplate, url, timeoutMs, serviceName)
    }
}
