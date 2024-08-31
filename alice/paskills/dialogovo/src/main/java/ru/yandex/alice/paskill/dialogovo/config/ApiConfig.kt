package ru.yandex.alice.paskill.dialogovo.config

import org.springframework.boot.context.properties.ConfigurationProperties
import org.springframework.boot.context.properties.ConstructorBinding

@ConstructorBinding
@ConfigurationProperties(prefix = "api-config")
data class ApiConfig(
    override val url: String,
    override val timeout: Int,
    override val connectTimeout: Int
) : EndpointConfig
