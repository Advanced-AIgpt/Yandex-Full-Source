package ru.yandex.alice.paskill.dialogovo.config

import org.springframework.boot.context.properties.ConfigurationProperties
import org.springframework.boot.context.properties.ConstructorBinding

@ConstructorBinding
@ConfigurationProperties(prefix = "dialogovo.teasers")
data class TeasersConfig(
    val skillRequestTimeout: Long,
    val maxNumberOfTeasers: Int,
    val skillsIds: List<String>,
)
