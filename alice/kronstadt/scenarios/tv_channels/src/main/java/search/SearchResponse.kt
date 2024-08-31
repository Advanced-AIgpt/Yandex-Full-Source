package ru.yandex.alice.kronstadt.scenarios.tv_channels.search

import com.fasterxml.jackson.annotation.JsonProperty

data class SearchResponse(
    val response: Response
) {
    data class Response(
        val results: List<Result> = listOf()
    )

    data class Result(
        val groups: List<Group> = listOf(),
        val found: Found?,

        @JsonProperty("groups-on-page")
        val groupsOnPage: Int?,
        val mode: Int?,
    )

    data class Found(
        val all: Int = 0,
        val phrase: Int = 0,
        val strict: Int = 0,
    )

    data class Group(
        val found: Found?,
        val relevance: Long?,
        @JsonProperty("doccount") val docCount: Int?,
        val documents: List<Document> = listOf()
    )

    data class Document(
        val url: String?,
        val properties: Properties?,
        val relevance: Long?,
    )

    data class Properties(
        @JsonProperty("i_ts") val ts: Long?,
        @JsonProperty("s_device_id") val deviceId: String?
    )
}
