package ru.yandex.alice.paskill.dialogovo.config

import org.springframework.boot.context.properties.ConfigurationProperties
import org.springframework.boot.context.properties.ConstructorBinding

@ConstructorBinding
@ConfigurationProperties(prefix = "dialogovo.widgets")
data class WidgetsConfig(
    val skillRequestTimeout: Long,
    val skillsIds: List<String>
)
