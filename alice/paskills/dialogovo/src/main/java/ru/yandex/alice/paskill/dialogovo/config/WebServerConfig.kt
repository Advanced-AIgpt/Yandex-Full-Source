package ru.yandex.alice.paskill.dialogovo.config

import org.springframework.boot.context.properties.ConfigurationProperties
import org.springframework.boot.context.properties.ConstructorBinding

@ConstructorBinding
@ConfigurationProperties(prefix = "web-server-config")
data class WebServerConfig(
    val maxThreads: Int,
    val minThreads: Int,
    val queueSize: Int,
)
