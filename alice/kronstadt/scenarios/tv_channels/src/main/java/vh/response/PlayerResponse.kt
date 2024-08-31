package ru.yandex.alice.kronstadt.scenarios.tv_channels.vh.response

import com.fasterxml.jackson.annotation.JsonInclude
import com.fasterxml.jackson.annotation.JsonProperty

data class PlayerResponse(
    val content: PlayerContent
)

data class PlayerContent(
    val title: String,
    @JsonProperty("onto_otype") val ontoType: String?,
    val years: String?,
    val genres: List<String>?,
    val duration: Int?,
    val series: Series?,
    @JsonProperty("ya_plus") val yaPlus: List<String>?,
    val ottParams: OttParams?,
    @JsonProperty("release_year") val releaseYear: Int?,
    @JsonProperty("restriction_age") val restrictionAge: Int,
    @JsonProperty("content_type_name") val contentTypeName: String,
    val thumbnail: String?,
    @JsonProperty("onto_poster") val ontoPoster: String?,
)

data class OttParams(
    val monetizationModel: String,
    val serviceName: String,
    @JsonInclude(JsonInclude.Include.NON_NULL) val kpId: String?,
    @JsonProperty("contentTypeID") val contentTypeId: Long?,
    val licenses: List<OttLicense>?,
    val reqid: String,
    val yandexuid: String,
    val uuid: String,
    val from: String,
    val subscriptionType: String?,
)

data class OttLicense(
    @JsonInclude(JsonInclude.Include.NON_NULL) val priceWithDiscount: Int?,
    val monetizationModel: String?,
    @JsonInclude(JsonInclude.Include.NON_NULL) val price: Int?,
    val primary: Boolean,
    @JsonInclude(JsonInclude.Include.NON_NULL) val active: Boolean?,
    @JsonInclude(JsonInclude.Include.NON_NULL) val purchaseTag: String?,
)

data class Series(
    val title: String,
    val id: String,
)
