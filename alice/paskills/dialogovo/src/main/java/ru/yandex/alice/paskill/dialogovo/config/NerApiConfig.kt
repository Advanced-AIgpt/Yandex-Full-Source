package ru.yandex.alice.paskill.dialogovo.config

import org.springframework.boot.context.properties.ConfigurationProperties
import org.springframework.boot.context.properties.ConstructorBinding

@ConstructorBinding
@ConfigurationProperties("ner-api")
data class NerApiConfig(
    override val url: String,
    override val timeout: Int,
    override val connectTimeout: Int,
    val cacheTtlSeconds: Int,
    val cacheSize: Int
) : EndpointConfig
