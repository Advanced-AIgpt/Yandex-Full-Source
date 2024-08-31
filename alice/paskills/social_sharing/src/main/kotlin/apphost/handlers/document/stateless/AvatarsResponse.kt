package ru.yandex.alice.social.sharing.apphost.handlers.document.stateless

import com.fasterxml.jackson.annotation.JsonProperty

internal data class AvatarsResponse(
    @JsonProperty("group-id") val groupId: Long,
    @JsonProperty("imagename") val imageName: String,
)
