package ru.yandex.alice.kronstadt.scenarios.contacts.mssngr.domain

import com.fasterxml.jackson.annotation.JsonProperty

const val RESPONSE_STATUS_OK = "ok"

data class UploadContactsResponse(
    val status: String,
    val data: Data,
) {
    data class Data(
        @JsonProperty("Status") val status: String?,
        val code: String?,
        val text: String?
    )
}
