package ru.yandex.alice.kronstadt.scenarios.tvmain

import org.springframework.context.annotation.Configuration
import org.springframework.context.annotation.PropertySource

@Configuration(proxyBeanMethods = false)
@PropertySource("classpath:/config/tv-main-config.yaml")
@PropertySource(
    "classpath:/config/tv-main-config-\${spring.profiles.active}.yaml",
    ignoreResourceNotFound = true
)
open class TvMainConfiguration {
}
