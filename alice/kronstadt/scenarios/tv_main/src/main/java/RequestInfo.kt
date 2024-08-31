package ru.yandex.alice.kronstadt.scenarios.tvmain

import com.fasterxml.jackson.annotation.JsonProperty

data class RequestInfo(
    @JsonProperty("apphost-reqid") val apphostReqid: String?,
    @JsonProperty("nonbanned_docs_count") val nonBannedCount: Int?,
    val lang: String?
)
