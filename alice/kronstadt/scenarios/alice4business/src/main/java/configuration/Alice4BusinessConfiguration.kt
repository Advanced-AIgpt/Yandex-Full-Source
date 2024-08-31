package ru.yandex.alice.kronstadt.scenarios.alice4business.configuration

import org.springframework.beans.factory.annotation.Value
import org.springframework.context.annotation.Bean
import org.springframework.context.annotation.Configuration
import org.springframework.context.annotation.PropertySource
import org.springframework.web.client.RestTemplate
import ru.yandex.alice.paskills.common.resttemplate.factory.RestTemplateClientFactory
import java.time.Duration

@PropertySource("classpath:config/alice4business-config.yaml")
@PropertySource(
    "classpath:config/alice4business-config-\${spring.profiles.active}.yaml",
    ignoreResourceNotFound = true
)
@Configuration
open class Alice4BusinessConfiguration {
    @Bean
    open fun alice4BusinessRestTemplate(
        restTemplateClientFactory: RestTemplateClientFactory,
        @Value("\${alice4-business-config.timeout}")
        timeout: Long,
        @Value("\${alice4-business-config.connectTimeout}")
        connectTimeout: Long
    ): RestTemplate =
        restTemplateClientFactory
            .serviceWebClientWithoutRetry(
                "alice4business",
                Duration.ofMillis((0.7 * timeout).toLong()),
                Duration.ofMillis(connectTimeout),
                Duration.ofMillis(timeout),
                10,
                true
            )
}
