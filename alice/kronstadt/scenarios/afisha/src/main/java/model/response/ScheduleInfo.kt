package ru.yandex.alice.kronstadt.scenarios.afisha.model.response

import com.fasterxml.jackson.annotation.JsonProperty

data class ScheduleInfo(
    val placePreview: String?,
    @JsonProperty("preview") val datePreview: Preview?
) {
    data class Preview(
        @JsonProperty("text") val date: String?
    )
}
