package ru.yandex.alice.paskill.dialogovo.config

import com.fasterxml.jackson.annotation.JsonProperty
import ru.yandex.alice.kronstadt.core.directive.Style

data class SearchAppStyles(
    val internal: Style,
    val external: Style,
    @JsonProperty("internal_dark") val internalDark: Style,
    @JsonProperty("external_dark") val externalDark: Style
)
