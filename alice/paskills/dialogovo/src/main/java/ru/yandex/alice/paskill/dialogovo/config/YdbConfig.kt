package ru.yandex.alice.paskill.dialogovo.config

import org.springframework.boot.context.properties.ConfigurationProperties
import org.springframework.boot.context.properties.ConstructorBinding

@ConstructorBinding
@ConfigurationProperties(prefix = "ydb")
data class YdbConfig(
    val warmUpOnStart: Boolean = false,
    val sessionPoolSizeMin: Int = 0,
    val sessionPoolSizeMax: Int = 0,
    val queryCacheSize: Int = 0,
    val maxRetries: Int = 0,
) {
    fun isWarmUpOnStart() = warmUpOnStart
}
