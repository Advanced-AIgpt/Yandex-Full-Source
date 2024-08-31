package ru.yandex.alice.paskill.dialogovo.service.recommender

import com.fasterxml.jackson.annotation.JsonInclude
import com.fasterxml.jackson.annotation.JsonProperty

@JsonInclude(JsonInclude.Include.NON_ABSENT)
class RecommenderRequestAttributes(
    @JsonProperty("alice_experiments") val experiments: Set<String> = emptySet(),
    @JsonProperty("user_data") val userData: UserData? = null
) {

    @JsonInclude(JsonInclude.Include.NON_ABSENT)
    class UserData @JvmOverloads constructor(
        val uuid: String,
        val puid: String? = null,
        @JsonProperty("client_id") val clientId: String? = null,
        @JsonProperty("device_id") val deviceId: String? = null
    )
}
