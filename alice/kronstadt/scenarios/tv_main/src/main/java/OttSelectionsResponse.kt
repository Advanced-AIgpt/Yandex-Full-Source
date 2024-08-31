package ru.yandex.alice.kronstadt.scenarios.tvmain

import com.fasterxml.jackson.annotation.JsonProperty
import com.fasterxml.jackson.annotation.JsonSubTypes
import com.fasterxml.jackson.annotation.JsonTypeInfo
import java.math.BigDecimal

data class ShowcaseGatewayDto(
    val status: String,
    val statusDescription: String?,
    val collections: List<SelectionGatewayDto>,
    val unavailabilityDetails: List<SelectionAvailabilityGatewayDto>?,
    val pagingMeta: SelectionPagingMetaGatewayDto?,
    val requestProfiling: Map<String, String> = mapOf()
) {}

data class SelectionPagingMetaGatewayDto(
    val from: Int?,
    val to: Int?,
    val hasMore: Boolean = false,
    val sessionId: String?,
)

data class SelectionGatewayDto(
    val data: List<SelectionMetaItemGatewayDto> = listOf(),
    val type: String,
    val title: String?,
    val group: String?,
    val selectionId: String,
    val comment: String?,
    val coverUrl: String?,
    val lowresCoverUrl: String?,
    val videoUrl: String?,
    val showTitle: Boolean?,
    val availability: SelectionAvailabilityGatewayDto?,
    val pagingMeta: SelectionPagingMetaGatewayDto,
    val upsaleSubscription: String?,
    val backgroundImage: String?,
    val targetWindowId: String?,
    val targets: String?,
)

data class SelectionAvailabilityGatewayDto(
    val status: String?,
    val details: String?
)

@JsonTypeInfo(use = JsonTypeInfo.Id.NAME, property = "type", visible = true)
@JsonSubTypes(
    JsonSubTypes.Type(
        FilmSelectionMetaItemGatewayDto::class,
        name = "ITEM_VIDEO",
        names = [
            "ITEM_FEATURE",
            "ITEM_ORIGINAL",
            "ITEM_ORIGINAL_ANNOUNCE",
            "ITEM_ANNOUNCE",
            "ITEM_CONTENT",
            "ITEM_CHANNEL",
            "ITEM_SEASON_ANNOUNCE",
        ]
    ),
    JsonSubTypes.Type(
        ChannelProgramSelectionMetaItemGatewayDto::class,
        name = "ITEM_CHANNEL_PROGRAM"
    ),
    JsonSubTypes.Type(
        EditorialFeatureSelectionMetaItemGatewayDto::class,
        name = "ITEM_EDITORIAL_FEATURE"
    ),
    JsonSubTypes.Type(
        EditorialFeatureVideoSelectionMetaItemGatewayDto::class,
        name = "ITEM_EDITORIAL_FEATURE_VIDEO"
    ),
    JsonSubTypes.Type(
        LinkSelectionMetaItemGatewayDto::class,
        name = "SNIPPET",
        names = ["SELECTION"]
    )
)
sealed class SelectionMetaItemGatewayDto(
    val id: String?,
    val title: String?,
    val type: String,
)

class FilmSelectionMetaItemGatewayDto(
    id: String?,
    title: String?,
    type: String,
    val filmId: String?,
    val kpId: Int?,
    val kpRating: Double?,
    val genres: List<String>?,
    val posterUrl: String?,
    val minPrice: Int?,
    val years: String?,
    val watchingOption: WatchingOption?,
    val horizontalPoster: String?,
    val coverUrl: String?,
    val shortDescription: String?,
    val editorAnnotation: String?,
    val logo: ImageTo?,
    val seasonsCount: Int?,
    val contentType: ContentType?,
    val restrictionAge: Int?,
    val countries: List<String>?,
    val medianColor: String?,
    val duration: Int?,
    val hasSubscription: Boolean?,
) : SelectionMetaItemGatewayDto(id, title, type)

class EditorialFeatureSelectionMetaItemGatewayDto(
    id: String?,
    type: String,
    val itemType: String?,
    val imageSizeType: String?,
    val imageUrl: String?,
    val entityType: String?,
    val entityId: String?,
    val entityName: String?,
) : SelectionMetaItemGatewayDto(id, null, type)

class EditorialFeatureVideoSelectionMetaItemGatewayDto(
    id: String?,
    type: String,
    val itemType: String?,
    val imageSizeType: String?,
    val imageUrl: String?,
    val entityType: String?,
    val entityId: String?,
    val entityName: String?,
    val videoContentId: String?,
    val videoUrl: String?
) : SelectionMetaItemGatewayDto(id, null, type)

class ChannelProgramSelectionMetaItemGatewayDto(
    id: String?,
    title: String?,
    type: String,
    val contentType: ContentType?,
    val tvChannelLogoUrl: String,
    val currentTime: String,
    val restrictionAge: Int?,
) : SelectionMetaItemGatewayDto(id, title, type)

class LinkSelectionMetaItemGatewayDto(
    id: String?,
    title: String?,
    type: String,
    val selectionId: String?,
    val imageUrl: String?,
    val squareImageUrl: String?,
    val foregroundImageUrl: String?,
    val backgroundImageUrl: String?,
    val videoUrl: String?,
    val targetWindowId: String?
) : SelectionMetaItemGatewayDto(id, title, type)

data class ImageTo(
    var url: String?,
    var width: Int?,
    var height: Int?,
)

data class PriceTo(
    val value: BigDecimal?,
    val currency: String?
)

enum class Subscription(val isYaPlus: Boolean, val rank: Int, val isForBelarus: Boolean) {
    YA_PREMIUM(false, 1, false),
    YA_PLUS_SUPER(true, 2, false),
    KP_BASIC(false, 3, false),
    YA_PLUS_KP(true, 4, true),
    YA_PLUS_KP_3M(true, 5, true),
    YA_PLUS(true, 6, false),
    YA_PLUS_3M(true, 7, false),
    NONE(false, 8, false)
}

class WatchingOption(
    val type: String?,
    val monetizations: List<String>?,
    val subscription: Subscription?,
    val minimumPrice: BigDecimal?,
    val minimumPriceDetails: PriceTo?,
    val purchased: Boolean?
)

enum class ContentType(val id: Int, val externalName: String) {
    @JsonProperty("channel")
    CHANNEL(2, "channel"),

    @JsonProperty("episode")
    CHANNEL_EPISODE(3, "episode"),

    @JsonProperty("tv-series")
    TV_SERIES(6, "vod-episode"),

    @JsonProperty("tv-season")
    SEASON(7, "tv-season"),

    @JsonProperty("type-folder")
    FOLDER(10, "type-folder"),

    @JsonProperty("ott-episode")
    EPISODE(21, "ott-episode"),

    @JsonProperty("ott-movie")
    MOVIE(20, "vod-library"),

    @JsonProperty("ott-sport-episode")
    SPORT_EPISODE(23, "ott-sport-episode"),

    @JsonProperty("ott-trailer")
    TRAILER(37, "ott-trailer"),

    @Deprecated("")
    @JsonProperty("ott-collection")
    COLLECTION(38, "ott-collection"),

    @JsonProperty("ott-collection-item")
    COLLECTION_ITEM(39, "ott-collection-item"),

    @JsonProperty("vod-broadcast-episode")
    BROADCAST(45, "broadcast"),

    @JsonProperty("kp-trailer")
    KP_TRAILER(47, "kp-trailer"),

    @JsonProperty("music-clip")
    MUSIC_CLIP(51, "music-clip"),

    @Deprecated("")
    @JsonProperty("ott-linked-video")
    LINKED_VIDEO(82, "ott-linked-video"),

    @Deprecated("")
    @JsonProperty("ott-music-karaoke")
    MUSIC_KARAOKE(83, "ott-music-karaoke"),

    @JsonProperty("ott-vod-library")
    VOD_LIBRARY(86, "ott-vod-library"),

    @JsonProperty("ott-linked-trailer")
    LINKED_TRAILER(87, "ott-linked-trailer"),

    @JsonProperty("ott-linked-music-clip")
    LINKED_MUSIC_CLIP(88, "ott-linked-music-clip"),

    @JsonProperty("ott-video-karaoke")
    VIDEO_KARAOKE(121, "ott-video-karaoke"),

    @JsonProperty("ott-video-episode")
    VIDEO_EPISODE(122, "ott-video-episode"),

    @JsonProperty("ott-video-program")
    VIDEO_PROGRAM(123, "ott-video-program"),

    @JsonProperty("ott-clips")
    CLIPS(220, "ott-clips");
}
