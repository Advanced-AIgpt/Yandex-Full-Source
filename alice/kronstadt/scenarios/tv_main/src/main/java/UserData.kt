package ru.yandex.alice.kronstadt.scenarios.tvmain

import com.fasterxml.jackson.annotation.JsonProperty

data class UserData(
    @JsonProperty("req_id") val reqId: String
)
