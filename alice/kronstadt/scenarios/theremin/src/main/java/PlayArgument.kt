package ru.yandex.alice.paskill.dialogovo.scenarios.theremin

import com.fasterxml.jackson.annotation.JsonProperty

data class PlayArgument(
    val mielophone: Boolean = false,
    @JsonProperty("beat_number") val beatNumber: Int?,
    @JsonProperty("beat_enum") val beatName: String?,
    @JsonProperty("beat_group") val beatGroup: String?,
    @JsonProperty("beat_group_index") val beatGroupIndex: Int?,
    @JsonProperty("beat_text") val beatText: String?,
)
