package ru.yandex.alice.paskill.dialogovo.config

import org.springframework.boot.context.properties.ConfigurationProperties
import org.springframework.boot.context.properties.ConstructorBinding

@ConstructorBinding
@ConfigurationProperties(prefix = "recommender-config")
data class RecommenderConfig(
    override val url: String,
    override val timeout: Int,
    override val connectTimeout: Int
) : EndpointConfig
