package ru.yandex.alice.paskill.dialogovo.service.recommender

import com.fasterxml.jackson.annotation.JsonProperty

data class RecommenderResponse(
    @JsonProperty("recommendation_type") val recommendationType: String,
    @JsonProperty("recommendation_source") val recommendationSource: String,
    val items: List<RecommenderItem> = emptyList(),
)
