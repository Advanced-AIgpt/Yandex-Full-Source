package ru.yandex.alice.kronstadt.scenarios.tv_channels.vh

import org.springframework.beans.factory.annotation.Value
import org.springframework.boot.web.client.RestTemplateBuilder
import org.springframework.context.annotation.Bean
import org.springframework.context.annotation.Configuration
import org.springframework.http.client.HttpComponentsClientHttpRequestFactory
import ru.yandex.alice.kronstadt.core.RequestContext
import ru.yandex.alice.paskills.common.logging.protoseq.ClientSetraceInterceptor
import ru.yandex.alice.paskills.common.resttemplate.factory.ClientHttpHeadersInterceptor
import ru.yandex.alice.paskills.common.resttemplate.factory.client.HttpClientFactory
import java.net.URI
import java.time.Duration
import java.util.Optional

@Configuration
open class VhClientConfiguration {

    @Value("\${tv-channels.vh-read-timeout}")
    private val readTimeout: Long = 3000

    @Value("\${tv-channels.vh-connect-timeout}")
    private val connectTimeout: Long = 30

    @Value("\${tv-channels.vh-max-connections}")
    private val maxConnections: Int = 50

    @Value("\${tv-channels.vh-url}")
    private lateinit var vhUrl: URI

    @Bean
    open fun vhClient(
        baseBuilder: RestTemplateBuilder,
        httpClientFactory: HttpClientFactory,
        requestContext: RequestContext,
        clientHttpHeadersInterceptor: ClientHttpHeadersInterceptor,

        ): VhClient {
        val client = httpClientFactory.builder()
            .connectTimeout(Duration.ofMillis(this.connectTimeout))
            .retryOnIO(2, false)
            .instrumented("http.out", "frontend_vh", false, Optional.empty())
            .maxConnectionCount(maxConnections)
            .maxConnectionsPerRoute(maxConnections)
            .disableRedirects()
            .disableCookieManagement()
            .build()

        val requestFactory = HttpComponentsClientHttpRequestFactory(client)

        val restTemplate = baseBuilder
            .requestFactory { requestFactory }
            .additionalInterceptors(
                clientHttpHeadersInterceptor,
                ClientSetraceInterceptor("frontend_vh", true),
            )
            .setReadTimeout(Duration.ofMillis(this.readTimeout))
            .setConnectTimeout(Duration.ofMillis(this.connectTimeout))
            .build()

        println(restTemplate)

        return VhClient(restTemplate, requestContext, vhUrl)
    }
}
