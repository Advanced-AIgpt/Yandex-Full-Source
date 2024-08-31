package ru.yandex.alice.paskill.dialogovo.config

import org.springframework.boot.context.properties.ConfigurationProperties
import org.springframework.boot.context.properties.ConstructorBinding
import java.net.URI

@ConstructorBinding
@ConfigurationProperties(prefix = "redirect-api")
data class RedirectApiConfig(
    val clientId: String,
    val key: String,
    val url: URI,
)
