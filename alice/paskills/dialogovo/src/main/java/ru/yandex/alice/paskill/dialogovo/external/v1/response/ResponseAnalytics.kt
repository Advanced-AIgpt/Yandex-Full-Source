package ru.yandex.alice.paskill.dialogovo.external.v1.response

import com.fasterxml.jackson.annotation.JsonInclude
import com.fasterxml.jackson.annotation.JsonProperty
import ru.yandex.alice.paskill.dialogovo.service.appmetrica.MetricaEvent

@JsonInclude(JsonInclude.Include.NON_NULL)
data class ResponseAnalytics(
    val scene: SceneAnalytics?,
    val events: List<MetricaEvent> = listOf(),
    @JsonProperty("sensitive_data") val sensitiveData: Boolean = false,
) {
    data class SceneAnalytics(val id: String)
}
