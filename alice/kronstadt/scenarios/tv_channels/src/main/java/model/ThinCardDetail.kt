package ru.yandex.alice.kronstadt.scenarios.tv_channels.model

import com.fasterxml.jackson.annotation.JsonInclude
import com.fasterxml.jackson.annotation.JsonProperty
import ru.yandex.alice.kronstadt.scenarios.tv_channels.vh.response.OttParams
import ru.yandex.alice.kronstadt.scenarios.tv_channels.vh.response.Series

data class ThinCardDetail(
    val title: String,
    @JsonInclude(JsonInclude.Include.NON_NULL) val years: String?,
    @JsonInclude(JsonInclude.Include.NON_NULL) val genres: String?,
    @JsonInclude(JsonInclude.Include.NON_NULL) val duration: Int?,
    @JsonInclude(JsonInclude.Include.NON_NULL) val series: Series?,
    @JsonInclude(JsonInclude.Include.NON_NULL) @JsonProperty("ya_plus") val yaPlus: List<String>?,
    @JsonProperty("ott_params") val ottParams: OttParams?,
    @JsonProperty("content_id") val contentId: String,
    @JsonInclude(JsonInclude.Include.NON_NULL) @JsonProperty("release_year") val releaseYear: Int?,
    // val showTvPromo: Boolean,
    @JsonInclude(JsonInclude.Include.NON_NULL) @JsonProperty("restriction_age") val restrictionAge: Int,
    @JsonProperty("content_type") val contentType: String,
    @JsonProperty("monetization_model") val monetizationModel: String?,
    val thumbnail: String?,
    @JsonInclude(JsonInclude.Include.NON_NULL) val poster: String?,
    // val releaseData: String,
    // val season: String,
    // val episodeNumber: String,
    // val startAt: String,
    @JsonProperty("player_id") val playerId: String,
    @JsonProperty("paid_channel_stub") val paidChannelStub: PaidChannelStub?,

    @JsonInclude(JsonInclude.Include.NON_NULL)
    @JsonProperty("paid_channel_stub_v2")
    val paidChannelStubV2: PaidChannelStubV2?,
    // val Poster: String,
)

