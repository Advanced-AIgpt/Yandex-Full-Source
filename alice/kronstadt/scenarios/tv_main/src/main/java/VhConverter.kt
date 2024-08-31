package ru.yandex.alice.kronstadt.scenarios.tvmain

import org.apache.logging.log4j.LogManager
import org.springframework.stereotype.Component
import org.springframework.web.util.UriComponents
import org.springframework.web.util.UriComponentsBuilder
import ru.yandex.alice.kronstadt.core.convert.ProtoUtil
import ru.yandex.alice.protos.data.appmetrika.AppMetricaProto
import ru.yandex.alice.protos.data.scenario.video.Gallery
import ru.yandex.alice.protos.data.tv.Carousel
import ru.yandex.alice.protos.data.tv.appmetrika.TvAppMetrikaPayloads
import ru.yandex.alice.protos.data.tv.appmetrika.TvAppMetrikaPayloads.TCardGeneralPayload
import ru.yandex.alice.protos.data.tv.appmetrika.TvAppMetrikaPayloads.TCarouselGeneralPayload
import ru.yandex.alice.protos.data.tv.home.TvHomeResultProto.THomeCarouselWrapper
import ru.yandex.alice.protos.data.tv.home.TvHomeResultProto.TTvFeedResultData
import ru.yandex.alice.protos.data.video.VideoProto
import ru.yandex.alice.protos.div.Div2CardProto

@Component
class VhConverter(private val protoUtils: ProtoUtil) {
    val PLANNED_TO_WATCH_CAROUSEL_ID = "hhpfxagheebrxcdchh"
    val CONTINUE_WATCH_CAROUSEL_ID = "delayed_tvo"
    val OTT_ALLOWED_TYPES = listOf<String>("PROMO", "SELECTION", "OTT_TOP", "EDITORIAL_FEATURE", "MULTISELECTION", "ORIGINALS")
    fun makeFeedResult(body: FeedResponse, tvContext: TvContext, div2Render: String?) =
        TTvFeedResultData.newBuilder().apply {
            cacheHash = body.cacheHash

            val offset: Int
            if (div2Render != null) {
                addCarousels(THomeCarouselWrapper.newBuilder().apply {
                    divCarouselBuilder.id = "FRONTEND_CATEG_PROMO_MIXED"
                    divCarouselBuilder.title = body.items[0].title
                    divCarouselBuilder.elementsCount = body.items[0].items().size
                    divCarouselBuilder.divCardJson = Div2CardProto.TDiv2Card.newBuilder()
                        .setStringBody(div2Render)
                        .build()
                }.build())
                offset = 1
            } else {
                offset = 0
            }

            addAllCarousels(body.items.drop(offset).mapIndexed { ind, it ->
                makeCarousel(
                    it, CarouselContext(
                        title = it.title ?: "unknown_gallery ${ind}",
                        carouselId = it.carouselId ?: "gallery_$ind",
                        carouselPosition = ind + 1 + offset,
                        tvContext = tvContext,
                    )
                )
            })
        }.build()

    fun makeFeedResult(ottResponse: ShowcaseGatewayDto, tvContext: TvContext, div2Render: String?) =
        TTvFeedResultData.newBuilder().apply {
            val offset: Int
            if (div2Render != null) {
                val firstCarousel = ottResponse.collections[0]
                addCarousels(THomeCarouselWrapper.newBuilder().apply {
                    divCarouselBuilder.id = "FRONTEND_CATEG_PROMO_MIXED"
                    divCarouselBuilder.title = firstCarousel.title
                    divCarouselBuilder.elementsCount = firstCarousel.data.size
                    divCarouselBuilder.divCardJson = Div2CardProto.TDiv2Card.newBuilder()
                        .setStringBody(div2Render)
                        .build()
                }.build())
                offset = 1
            } else {
                offset = 0
            }

            addAllCarousels(ottResponse.collections.filter { OTT_ALLOWED_TYPES.contains(it.type) }.mapIndexed { ind, it ->
                makeCarousel(
                    it, CarouselContext(
                        title = it.title ?: "unknown_gallery $ind",
                        carouselId = it.selectionId,
                        carouselPosition = ind + 1 + offset,
                        tvContext = tvContext,
                    )
                )
            })
        }.build()

    fun makeOttCarousel(carousel: SelectionGatewayDto, carouselContext: CarouselContext): List<Gallery.TGalleryData.TCard> =
        carousel.data.filterIsInstance<FilmSelectionMetaItemGatewayDto>().mapIndexed { ind, episodeInfo ->
            Gallery.TGalleryData.TCard.newBuilder().apply {
                episodeInfo.posterUrl
                logoAlpha = 0.0
                episodeInfo.logo?.url
                legalLogoAlpha = 0.0
                metaText = " "
                subscriptionText = " "

                requiredLicenseType = kotlin.runCatching { Gallery.TGalleryData.TCard.LicenseType.valueOf(
                    episodeInfo.watchingOption?.monetizations?.first().orEmpty()) }.getOrDefault(Gallery.TGalleryData.TCard.LicenseType.UNKNOWN)
                isContentAvailable = episodeInfo.watchingOption?.purchased == true || episodeInfo.hasSubscription == true

                rating = (episodeInfo.kpRating?.toFloat() ?: 0) as Float
                ratingColor = "0xb3b3b3"

                ontoId = "" // Ott does not has ontoId
                descriptionText = episodeInfo.shortDescription.orEmpty()
                contentId = episodeInfo.id
                contentType = episodeInfo.contentType?.externalName.orEmpty()
                addAllGenres(episodeInfo.genres)
                releaseYear = episodeInfo.years?.split('-')?.toTypedArray()?.first()?.toInt() ?: 0
                durationSeconds = episodeInfo.duration?.toLong() ?: 0
                ageLimit = episodeInfo.restrictionAge!!

                episodeInfo.logo?.let { logoUrls = buildImage(it.url.toString(), LOGO_IMAGE_RESOLUTIONS) }
                titleText = episodeInfo.title
                episodeInfo.logo?.url.orEmpty().let {
                    legalLogoUrls = buildImage(it, LEGAL_LOGO_IMAGE_RESOLUTIONS)
                }
                thumbnailUrls = buildImage(episodeInfo.horizontalPoster.orEmpty(), THUMBNAIL_IMAGE_RESOLUTIONS)
                episodeInfo.kpRating
                episodeInfo.countries?.first()
                onVhContentCardOpenedEvent = protoUtils.messageToStruct(
                    createAppmetrikaOpenEvent(
                        episodeInfo,
                        CardContext(carouselContext, ind + 1),
                    )
                )
                carouselItemGeneralAnalyticsInfoPayload = protoUtils.messageToStruct(
                    createAppmetrikaCardEventGeneralPayload(
                        documentContentId = episodeInfo.id.orEmpty(),
                        documentContentType = episodeInfo.contentType.toString(),
                        carouselContext = carouselContext,
                    )
                )
                getSubscriptionTypes(episodeInfo).let {
                    addAllSubscriptionTypes(it)
                }

            }.build()
        }

    fun makeCarousel(vhBody: VhCarousel, carouselContext: CarouselContext): THomeCarouselWrapper {
        val wrapper = THomeCarouselWrapper.newBuilder()

        wrapper.homeCarouselBuilder.apply {
            id = vhBody.carouselId?: carouselContext.carouselId
            if (id == PLANNED_TO_WATCH_CAROUSEL_ID || id == CONTINUE_WATCH_CAROUSEL_ID) {
                wrapper.ttlInfoBuilder.apply {
                    hasTtl = true
                    ttlMillis = 0
                }
            }
            title = vhBody.title?: carouselContext.title
            if (vhBody.items().size >= carouselContext.tvContext.limit && !vhBody.cacheHash.isNullOrEmpty()) {
                cacheHash = vhBody.cacheHash
            }

            val wrappedItems = vhBody.items()
                .mapIndexed() { ind, item ->
                    val ctx = CardContext(carouselContext = carouselContext, cardPosition = ind + 1)
                    Carousel.TCarouselItemWrapper.newBuilder()
                        .setVideoItem(toOttVideoItem(item, ctx))
                        .setCarouselItemGeneralAnalyticsInfoPayload(
                            protoUtils.messageToStruct(createAppmetrikaCardEventGeneralPayload(
                                item.contentId,
                                item.contentTypeName,
                                carouselContext
                            ))
                        )
                        .build()
                }

            addAllItems(wrappedItems)
        }
        wrapper.analyticsInfoPayload = protoUtils.messageToStruct(createCarouselGeneralPayload(carouselContext))
        return wrapper.build()
    }

    private fun makeCarousel(ottBody: SelectionGatewayDto, carouselContext: CarouselContext): THomeCarouselWrapper {
        val wrapper = THomeCarouselWrapper.newBuilder()

        wrapper.homeCarouselBuilder.apply {
            id = ottBody.selectionId
            if (id == PLANNED_TO_WATCH_CAROUSEL_ID || id == CONTINUE_WATCH_CAROUSEL_ID) {
                wrapper.ttlInfoBuilder.apply {
                    hasTtl = true
                    ttlMillis = 0
                }
            }
            title = ottBody.title?: carouselContext.title

            val wrappedItems = ottBody.data.filterIsInstance<FilmSelectionMetaItemGatewayDto>()
                .mapIndexed() { ind, item ->
                    val ctx = CardContext(carouselContext = carouselContext, cardPosition = ind + 1)
                    Carousel.TCarouselItemWrapper.newBuilder()
                        .setVideoItem(
                            toOttVideoItem(item, ctx)
                        ).setCarouselItemGeneralAnalyticsInfoPayload(
                            protoUtils.messageToStruct(createAppmetrikaCardEventGeneralPayload(
                                item.id.orEmpty(),
                                item.contentType?.externalName.orEmpty(),
                                carouselContext
                            ))
                        )
                        .build()
                }

            addAllItems(wrappedItems)
        }
        wrapper.analyticsInfoPayload = protoUtils.messageToStruct(createCarouselGeneralPayload(carouselContext))
        return wrapper.build()
    }

    private fun toOttVideoItem(
        episode: EpisodeInfo,
        ctx: CardContext
    ): VideoProto.TOttVideoItem.Builder = VideoProto.TOttVideoItem.newBuilder().apply {
        id = episode.contentId
        providerItemId = episode.contentId
        contentType = episode.contentTypeName
        title = episode.title
        episode.description?.let { description = it }
        ageLimit = episode.restrictionAge.toString()
        duration = episode.duration
        episode.ratingKp?.let { rating = it }
        miscIdsBuilder.ontoId = episode.ontoId
        onVhContentCardOpenedEvent = createAppmetrikaOpenEvent(episode, ctx)
        buildImage(fixSchema(episode.ontoPoster), POSTER_IMAGE_RESOLUTIONS)?.let { poster = it }
        buildImage(fixSchema(episode.thumbnail), THUMBNAIL_IMAGE_RESOLUTIONS)?.let { thumbnail = it }
        vhLicences = getVhLicencesInfo(episode)
        genres = episode.genres.joinToString(", ")
        episode.releaseYear?.let { releaseYear = it }
        episode.progress?.let { progress = it }
    }

    private fun toOttVideoItem(
        item: FilmSelectionMetaItemGatewayDto,
        ctx: CardContext
    ): VideoProto.TOttVideoItem.Builder =  VideoProto.TOttVideoItem.newBuilder().apply{
        id = item.id
        providerItemId = item.id
        contentType = item.contentType?.externalName.orEmpty()
        title = item.title
        description = item.shortDescription
        ageLimit = item.restrictionAge.toString()
        duration = item.duration?: 0
        rating = item.kpRating?: 0.0
        onVhContentCardOpenedEvent = createAppmetrikaOpenEvent(item, ctx)
        buildImage(fixSchema(item.posterUrl.orEmpty()), POSTER_IMAGE_RESOLUTIONS)?.let { poster = it }
        buildImage(fixSchema(item.horizontalPoster.orEmpty()), THUMBNAIL_IMAGE_RESOLUTIONS)?.let { thumbnail = it }
        vhLicences = getVhLicencesInfo(item.watchingOption, item.contentType)
        genres = item.genres?.joinToString(", ") ?: ""
        releaseYear = item.years?.split("-")?.firstOrNull()?.toIntOrNull()?: 0 // yeas are 2001-2005. Take first
//        episode.progress?.let { progress = it } // TODO no progress?
    }

    fun getSubscriptionTypes(episode: EpisodeInfo): List<String>? =
        episode.ottParams.licenses.mapNotNull { license ->
            if (license is SvodOttLicense && license.purchaseTag != null && PURCHASE_MAPPING.containsKey(license.purchaseTag)) {
                PURCHASE_MAPPING[license.purchaseTag]
            } else {
                null
            }
        }.takeIf { it.isNotEmpty() }

    fun getSubscriptionTypes(episode: FilmSelectionMetaItemGatewayDto): List<String> =
        listOf(episode.watchingOption?.subscription.toString() )

    private fun getVhLicencesInfo(episode: EpisodeInfo): VideoProto.TVHLicenceInfo {
        return if (episode.ottParams.licenses.isNotEmpty()) {
            val licenses = episode.ottParams.licenses
            VideoProto.TVHLicenceInfo.newBuilder().apply {
                licenses.forEach { license ->
                    when (license) {
                        is AvodOttLicense -> avod = 1
                        is TvodOttLicense -> {
                            tvod = 1
                            if (license.active) {
                                userHasTvod = 1
                            }
                        }
                        is EstOttLicense -> {
                            est = 1
                            if (license.active) {
                                userHasEst = 1
                            }
                        }
                        is SvodOttLicense -> {
                            if (license.purchaseTag!= null && PURCHASE_MAPPING.containsKey(license.purchaseTag)) {
                                addSvod(PURCHASE_MAPPING[license.purchaseTag])
                            }
                        }
                        else -> {}
                    }
                }
                when (episode.contentTypeName) {
                    "vod-episode" -> contentType = "MOVIE"
                    "vod-library" -> contentType = "TV_SERIES"
                    else -> logger.info("Unknown episode content type name: ${episode.contentTypeName}")
                }
            }.build()
        } else VideoProto.TVHLicenceInfo.getDefaultInstance()
    }

    private fun getVhLicencesInfo(watchOption: WatchingOption?, itemContentType: ContentType?): VideoProto.TVHLicenceInfo {
        if (watchOption != null) {
            if (watchOption.monetizations?.isNotEmpty() == true) {
                return VideoProto.TVHLicenceInfo.newBuilder().apply {
                    watchOption.monetizations.forEach { license ->
                        when (license) {
                            "AVOD" -> avod = 1
                            "TVOD" -> {
                                tvod = 1
                                if (watchOption.purchased == true) {
                                    userHasTvod = 1
                                }
                            }
                            "EST" -> {
                                est = 1
                                if (watchOption.purchased == true) {
                                    userHasEst = 1
                                }
                            }
                            "SVOD" -> {
                                if (watchOption.subscription != null) {
                                    addSvod(watchOption.subscription.name)
                                }
                            }
                            else -> {}
                        }
                    }
                    if (itemContentType != null) {
                        contentType = itemContentType.name
                    }
                }.build()
            }
        }
        return VideoProto.TVHLicenceInfo.getDefaultInstance()
    }

    private val VH_CARD_ALLOWED_FROM = setOf(
        "show_more_items",
        "film_contnet_card",
        "episode_content_card",
        "search_content_card",
        "search_collection_page",
    )

    fun createAppmetrikaOpenEvent(
        episode: EpisodeInfo,
        ctx: CardContext
    ): AppMetricaProto.TAppMetrikaEvent {
        val cardOpenedPayload = TvAppMetrikaPayloads.ThContentCardOpenedPayload.newBuilder().apply {
            title = episode.title
            contentType = episode.contentTypeName

            deviceId = ctx.carouselContext.tvContext.deviceId
            contentId = episode.contentId
            durationSec = episode.duration.toString()
            genres = episode.genres.joinToString(separator = ", ")

            startupPlaceBuilder.carouselName = ctx.carouselContext.title
            startupPlaceBuilder.from = ctx.carouselContext.tvContext.screenId

            if (VH_CARD_ALLOWED_FROM.contains(ctx.carouselContext.tvContext.screenId) && ctx.carouselContext.tvContext.parentScreenId != null) {
                startupPlaceBuilder.parentFrom = ctx.carouselContext.tvContext.parentScreenId
            }
            startupPlaceBuilder.parentId = ctx.carouselContext.carouselId
            startupPlaceBuilder.carouselPosition = ctx.carouselContext.carouselPosition.toString()
            startupPlaceBuilder.contentCardPosition = ctx.cardPosition.toString()

        }.build()

        return AppMetricaProto.TAppMetrikaEvent.newBuilder()
            .setName("vh_content_card_opened")
            .setPayload(protoUtils.messageToStruct(cardOpenedPayload))
            .build()
    }

    fun createAppmetrikaOpenEvent(
        ottEpisode: FilmSelectionMetaItemGatewayDto,
        ctx: CardContext
    ): AppMetricaProto.TAppMetrikaEvent {
        val cardOpenedPayload = TvAppMetrikaPayloads.ThContentCardOpenedPayload.newBuilder().apply {
            title = ottEpisode.title.orEmpty()
            contentType = ottEpisode.contentType.toString()

            deviceId = ctx.carouselContext.tvContext.deviceId
            contentId = ottEpisode.id.orEmpty()
            durationSec = ottEpisode.duration.toString()
            genres = ottEpisode.genres?.joinToString(separator = ", ").orEmpty()

            startupPlaceBuilder.carouselName = ctx.carouselContext.title
            startupPlaceBuilder.from = ctx.carouselContext.tvContext.screenId

            if (VH_CARD_ALLOWED_FROM.contains(ctx.carouselContext.tvContext.screenId) && ctx.carouselContext.tvContext.parentScreenId != null) {
                startupPlaceBuilder.parentFrom = ctx.carouselContext.tvContext.parentScreenId
            }
            startupPlaceBuilder.parentId = ctx.carouselContext.carouselId
            startupPlaceBuilder.carouselPosition = ctx.carouselContext.carouselPosition.toString()
            startupPlaceBuilder.contentCardPosition = ctx.cardPosition.toString()

        }.build()

        return AppMetricaProto.TAppMetrikaEvent.newBuilder()
            .setName("ott_content_card_opened")
            .setPayload(protoUtils.messageToStruct(cardOpenedPayload))
            .build()
    }

    fun createAppmetrikaCardEventGeneralPayload(
        documentContentId: String,
        documentContentType: String,
        carouselContext: CarouselContext
    ) = TCardGeneralPayload.newBuilder().apply {
        contentId = documentContentId
        contentType = documentContentType
        parentId = carouselContext.carouselId
        quasarDeviceId = carouselContext.tvContext.deviceId
        reqId = carouselContext.tvContext.requestId
        appHostReqId = carouselContext.tvContext.apphostRequestId
        carouselName = carouselContext.title
    }.build()

    private fun createCarouselGeneralPayload(
        ctx: CarouselContext
    ) = TCarouselGeneralPayload.newBuilder().apply {
        carouselId = ctx.carouselId
        carouselTitle = ctx.title
        verticalPosition = ctx.carouselPosition
    }.build()

    private fun getMdsUrlInfo(uri: String): UriComponents {
        val builder = UriComponentsBuilder.fromUriString(uri)
        val components = builder.build()
        if (components.host == "avatars.mds.yandex.net") {
            builder.scheme("https")
            builder.port(null)
            return builder.build()
        } else {
            return components
        }
    }

    fun buildImage(uri: String, resolution: List<String>): VideoProto.TAvatarMdsImage? {
        return getMdsUrlInfo(uri).let { mdsInfo ->
            VideoProto.TAvatarMdsImage.newBuilder().apply {
                baseUrl = mdsInfo.toUriString().substringBeforeLast('/') + "/"
                addAllSizes(resolution)
            }.build()
        }
    }

    fun fixSchema(uri: String): String {

        if (uri.isNotEmpty()) {
            val builder = UriComponentsBuilder.fromUriString(uri)
            val components = builder.build()

            if (components.scheme == null) {
                return builder.scheme("https").toUriString()
            }
        }
        return uri
    }

    companion object {
        private val logger = LogManager.getLogger(VhConverter::class.java)
        internal val POSTER_IMAGE_RESOLUTIONS = listOf("1920x1080", "120x90", "400x300", "360x540", "orig")
        internal val THUMBNAIL_IMAGE_RESOLUTIONS = listOf("1920x1080", "160x90", "720x360", "960x540", "orig")
        internal val LOGO_IMAGE_RESOLUTIONS = listOf("800x200", "orig")
        internal val LEGAL_LOGO_IMAGE_RESOLUTIONS = listOf("544x306", "orig")
        internal val PURCHASE_MAPPING = mapOf(
            "plus" to "YA_PLUS",
            "kp-basic" to "KP_BASIC",
            "kp-amediateka" to "YA_PREMIUM",
            "super-plus" to "YA_PLUS_SUPER",
        )
    }
}
