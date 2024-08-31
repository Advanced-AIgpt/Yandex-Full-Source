package ru.yandex.alice.kronstadt.scenarios.tv_channels

import org.springframework.context.annotation.Configuration
import org.springframework.context.annotation.PropertySource

@Configuration(proxyBeanMethods = false)
@PropertySource("classpath:/config/tv-channel-indexer-config.yaml")
@PropertySource(
    "classpath:/config/tv-channel-indexer-config-\${spring.profiles.active}.yaml",
    ignoreResourceNotFound = true
)
open class TvChannelIndexerConfiguration
