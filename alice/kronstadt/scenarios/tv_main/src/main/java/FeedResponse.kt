package ru.yandex.alice.kronstadt.scenarios.tvmain

import com.fasterxml.jackson.annotation.JsonProperty

data class FeedResponse(
    @JsonProperty("cache_hash") val cacheHash: String?,
    @JsonProperty(required = false) val items: List<VhCarousel> = listOf(),
    @JsonProperty("reqid") val reqid: String?,
    @JsonProperty("request-info") val requestInfo: RequestInfo?,
)
