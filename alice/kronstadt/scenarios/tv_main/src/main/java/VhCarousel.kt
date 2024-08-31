package ru.yandex.alice.kronstadt.scenarios.tvmain

import com.fasterxml.jackson.annotation.JsonIgnore
import com.fasterxml.jackson.annotation.JsonProperty
import com.fasterxml.jackson.annotation.JsonTypeInfo
import com.fasterxml.jackson.annotation.JsonTypeName
import com.fasterxml.jackson.databind.annotation.JsonDeserialize

data class VhCarousel(
    @JsonProperty("carousel_id") val carouselId: String?,
    @JsonProperty("title") val title: String?,
    @JsonProperty("cache_hash") val cacheHash: String?,
    @JsonDeserialize(contentUsing = VhDocumentDeserializer::class)
    @JsonProperty("includes") val includes: List<BaseDocument> = listOf(),
    @JsonDeserialize(contentUsing = VhDocumentDeserializer::class)
    @JsonProperty("set") val set: List<BaseDocument> = listOf(),

    // only for carousel_videohub
    @JsonProperty("reqid") val reqid: String?,
    @JsonProperty("request_info") val requestInfo: RequestInfo?,
    @JsonProperty("user_data") val userData: UserData?
) {
    @JsonIgnore
    fun items(): List<EpisodeInfo> = (includes.takeIf { it.isNotEmpty() } ?: set).filterIsInstance<EpisodeInfo>()
}

sealed interface BaseDocument {
}

data class BannedDocument(
    val banned: Boolean
): BaseDocument

data class ThumbnailLogo(
    val url: String,
    @JsonProperty("formFactor") val formFactor: String,
)

data class EpisodeInfo(
    @JsonProperty("ottParams") val ottParams: OttParams,
    @JsonProperty("content_id") val contentId: String,
    @JsonProperty("content_type_name") val contentTypeName: String,
    val title: String,
    val description: String?,
    @JsonProperty("short_description") val shortDescription: String?,
    @JsonProperty("restriction_age") val restrictionAge: Int,
    val duration: Int,
    @JsonProperty("rating_kp") val ratingKp: Double?,
    @JsonProperty("onto_id") val ontoId: String?,
    @JsonProperty("onto_poster") val ontoPoster: String,
    val thumbnail: String,
    val cover: String,
    val genres: List<String> = listOf(),
    @JsonProperty("release_year") val releaseYear: Int?,
    @JsonProperty("progress") val progress: Int?,
    val logo: String?,
    val thumbnailLogos: List<ThumbnailLogo> = listOf(),
    val countries: String?,
) : BaseDocument

data class StreamsInfo(
    val options: List<String> = listOf(),
    val thumbnail: String?,
    val url: String,
    val title: String?,
)

data class VodEpisodeInfo(
    @JsonProperty("blogger_id") val bloggerId: String?,
    @JsonProperty("supertag_title") val supertagTitle: String?,
    @JsonProperty("release_date") val releaseDate: String?,
    val streams: List<StreamsInfo> = listOf(),
    val duration: Int,
    @JsonProperty("last_interaction_time")val lastInteractionTime: Int?,
    @JsonProperty("progress") val progress: Int?,
    @JsonProperty("release_date_ut") val releaseDateUt: Int?,
    @JsonProperty("content_type_name") val contentTypeName: String,
    @JsonProperty("content_url") val contentUrl: String?,
    val thumbnail: String,
    val blacked: Boolean,
    val internal: Boolean?,
    @JsonProperty("can_play_on_station") val canPlayOnStation: Boolean?,
    @JsonProperty("embed_url") val embedUrl: String?,
    @JsonProperty("content_id") val contentId: String,
    val title: String,
    val description: String?,
    @JsonProperty("has_cachup") val hasCachup: Boolean?,
    @JsonProperty("update_time") val updateTime: Int?,
    val puid: String?,
    @JsonProperty("start_at") val startAt: Int?,
) : BaseDocument

data class OttParams(
    val from: String?,
    val uuid: String?,
    val puid: String?,
    val reqid: String?,
    val kpId: String?,
    @JsonProperty("monetizationModel") val monetizationModel: String,
    @JsonProperty("serviceName") val serviceName: String?,
    @JsonProperty("subscriptionType") val subscriptionType: String?,
    val licenses: List<OttLicense> = listOf()
)

@JsonTypeInfo(
    property = "monetizationModel",
    use = JsonTypeInfo.Id.NAME,
    visible = false,
    defaultImpl = UnknownOttLicense::class
)
sealed interface OttLicense {
    val active: Boolean
}

object UnknownOttLicense : OttLicense {
    override val active: Boolean = false
}

@JsonTypeName("AVOD")
data class AvodOttLicense(
    override val active: Boolean,
    val primary: Boolean?
) : OttLicense

@JsonTypeName("TVOD")
data class TvodOttLicense(
    override val active: Boolean,
    val price: Any? = null,
    val primary: Boolean?,
    @JsonProperty("price_with_discount") val priceWithDiscount: Any? = null,
    @JsonProperty("purchaseDate") val purchaseDate: Any? = null,
) : OttLicense

@JsonTypeName("EST")
data class EstOttLicense(
    override val active: Boolean,
    val price: Any? = null,
    val primary: Boolean?,
    @JsonProperty("price_with_discount") val priceWithDiscount: Any? = null,
    @JsonProperty("purchaseDate") val purchaseDate: Any? = null,
) : OttLicense

@JsonTypeName("SVOD")
data class SvodOttLicense(
    override val active: Boolean,
    val primary: Boolean?,
    @JsonProperty("purchaseTag") val purchaseTag: String?
) : OttLicense
