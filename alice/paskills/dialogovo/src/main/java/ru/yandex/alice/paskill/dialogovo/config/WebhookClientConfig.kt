package ru.yandex.alice.paskill.dialogovo.config

import org.springframework.boot.context.properties.ConfigurationProperties
import org.springframework.boot.context.properties.ConstructorBinding

@ConstructorBinding
@ConfigurationProperties(prefix = "webhook-client-config")
data class WebhookClientConfig(
    val maxResponseSize: Long,
    val connectTimeout: Int,
    val readTimeout: Int,
    val fullRequestTimeout: Int,
)
