package ru.yandex.alice.kronstadt.scenarios.tv_channels.indexer

import com.fasterxml.jackson.annotation.JsonProperty

//https://wiki.yandex-team.ru/jandekspoisk/saas/datadelivery/pq/demon-dlja-zapisi-v-saas-cherez-lb/#formatotvetanazaproszapisi
data class IndexerResponseItem(
    val comment: String?,
    val written: Boolean = false,
    @JsonProperty("user_error") val userError: Boolean = false,
)
