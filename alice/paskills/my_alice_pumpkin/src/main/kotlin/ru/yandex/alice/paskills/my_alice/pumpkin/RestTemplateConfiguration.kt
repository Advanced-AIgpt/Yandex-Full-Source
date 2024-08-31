package ru.yandex.alice.paskills.my_alice.pumpkin

import org.apache.http.client.HttpClient
import org.apache.http.impl.client.HttpClientBuilder
import org.springframework.beans.factory.annotation.Value
import org.springframework.context.annotation.Bean
import org.springframework.context.annotation.Configuration
import org.springframework.http.client.HttpComponentsClientHttpRequestFactory
import org.springframework.web.client.RestTemplate

@Configuration
open class RestTemplateConfiguration {

    @Bean
    open fun restTemplate(
        @Value("\${pumpkin.source.connect_timeout.ms}") connectTimeout: Int,
        @Value("\${pumpkin.source.read_timeout.ms}") readTimeout: Int,
    ): RestTemplate {
        val httpRequestFactory = HttpComponentsClientHttpRequestFactory()
        httpRequestFactory.setConnectTimeout(connectTimeout)
        httpRequestFactory.setReadTimeout(readTimeout)
        val httpClient: HttpClient = HttpClientBuilder.create()
            .disableCookieManagement()
            .disableRedirectHandling()
            .disableDefaultUserAgent()
            .useSystemProperties()
            .build()
        httpRequestFactory.httpClient = httpClient
        return RestTemplate(httpRequestFactory)
    }

}
