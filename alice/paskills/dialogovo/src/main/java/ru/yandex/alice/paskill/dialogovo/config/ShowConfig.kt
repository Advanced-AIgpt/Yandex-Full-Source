package ru.yandex.alice.paskill.dialogovo.config

import com.fasterxml.jackson.annotation.JsonProperty
import org.springframework.boot.context.properties.ConfigurationProperties
import org.springframework.boot.context.properties.ConstructorBinding

@ConstructorBinding
@ConfigurationProperties(prefix = "show")
data class ShowConfig(
    @JsonProperty("yt") val yt: ShowYtConfig,
    @JsonProperty("experiments") val experiments: List<String>,
    @JsonProperty("skillRequestTimeout") val skillRequestTimeout: Long,
    @JsonProperty("ytSaveTimeout") val ytSaveTimeout: Long,
    @JsonProperty("ytTtlInHours") val ytTtlInHours: Long
) {
    data class ShowYtConfig(val cluster: String, val directory: String)
}
