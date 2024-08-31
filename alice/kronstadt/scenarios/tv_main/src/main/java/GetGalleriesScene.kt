package ru.yandex.alice.kronstadt.scenarios.tvmain

import NAppHostHttp.Http.THttpResponse
import com.google.common.net.HttpHeaders
import com.google.protobuf.Empty
import com.yandex.div.dsl.serializer.toJsonString
import org.apache.logging.log4j.LogManager
import org.springframework.beans.factory.annotation.Value
import org.springframework.stereotype.Component
import org.springframework.web.util.UriComponentsBuilder
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RelevantResponse
import ru.yandex.alice.kronstadt.core.RequestContext
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody
import ru.yandex.alice.kronstadt.core.ScenePrepareBuilder
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.convert.ProtoUtil
import ru.yandex.alice.kronstadt.core.directive.GrpcDirective
import ru.yandex.alice.kronstadt.core.layout.Layout
import ru.yandex.alice.kronstadt.core.scenario.AbstractScene
import ru.yandex.alice.kronstadt.core.scenario.ContinuingSceneWithPrepare
import ru.yandex.alice.kronstadt.core.scenario.SceneWithPrepare
import ru.yandex.alice.kronstadt.core.tvm.USER_TICKET_HEADER
import ru.yandex.alice.divkttemplates.tvmain.TvMainTemplate
import ru.yandex.alice.megamind.protos.common.FrameProto.TGetVideoGalleriesSemanticFrame
import ru.yandex.alice.paskills.common.apphost.http.HttpRequest
import ru.yandex.alice.paskills.common.apphost.spring.ApphostController
import ru.yandex.alice.protos.api.renderer.Api.TDivRenderData
import ru.yandex.alice.protos.data.scenario.video.Gallery.TGalleryData
import ru.yandex.alice.protos.data.scenario.video.Gallery.TGalleryData.TCard
import ru.yandex.alice.protos.data.tv.home.TvHomeResultProto
import ru.yandex.monlib.metrics.histogram.Histograms
import ru.yandex.monlib.metrics.registry.MetricRegistry
import java.util.Optional
import kotlin.reflect.KClass
import kotlin.time.DurationUnit
import kotlin.time.ExperimentalTime
import kotlin.time.measureTimedValue

internal const val USE_OTT_API_MAIN = "use_ott_api_main"

@Component
@ApphostController(path = "/kronstadt/scenario/tv_main/grpc")
class GetGalleriesScene(
    private val requestContext: RequestContext,
    private val vhConverter: VhConverter,
    private val protoUtils: ProtoUtil,
    metricRegistry: MetricRegistry,
    @Value("\${tv-main.ott-api.service_id}") private val ottApiServiceId: Int,
    @Value("\${tv-main.ott-api.selection_window_id}") private val ottApiSelectionWindowId: String
) :
    AbstractScene<Any, TGetVideoGalleriesSemanticFrame>(
        "get_galleries",
        TGetVideoGalleriesSemanticFrame::class
    ), SceneWithPrepare<Any, TGetVideoGalleriesSemanticFrame>,
    ContinuingSceneWithPrepare<Any, TGetVideoGalleriesSemanticFrame, Empty> {

    private val PROCESSING_DURATION_HIST = metricRegistry.histogramRate(
        "scenario.tv_main.processing_duration"
    ) { Histograms.exponential(22, 1.5, 500.0) }

    private val NO_VH_RESPONSE_ERROR_RATE = metricRegistry.rate("no_vh_response")
    private val NO_OTT_RESPONSE_ERROR_RATE = metricRegistry.rate("no_ott_response")

    private fun renderDataForFirstCarousel(feed: FeedResponse, tvContext: TvContext): TDivRenderData {

        val carousel = feed.items[0]

        val carouselContext = CarouselContext(
            title = carousel.title ?: "Unknown carousel",
            carouselId = carousel.carouselId ?: "Unknown carousel ID",
            carouselPosition = 0,
            tvContext = tvContext,
        )

        val cards: List<TCard> = carousel.items().mapIndexed { ind, episodeInfo ->
            TCard.newBuilder().apply {
                episodeInfo.logo?.let { logoUrl = it }
                logoAlpha = 0.0
                episodeInfo.thumbnailLogos.firstOrNull()?.url?.let { legalLogoUrl = it }
                legalLogoAlpha = 0.0
                metaText = " "
                subscriptionText = " "

                requiredLicenseType =
                    kotlin.runCatching { TCard.LicenseType.valueOf(episodeInfo.ottParams.monetizationModel) }
                        .getOrDefault(TCard.LicenseType.UNKNOWN)
                isContentAvailable = episodeInfo.ottParams.licenses.any { it.active }

                ratingText = rating.toString()
                ratingColor = "0xb3b3b3"

                episodeInfo.ontoId?.let { ontoId = it }
                descriptionText = episodeInfo.shortDescription ?: episodeInfo.description ?: ""
                contentId = episodeInfo.contentId
                contentType = episodeInfo.contentTypeName
                addAllGenres(episodeInfo.genres)
                episodeInfo.releaseYear?.let { releaseYear = it }
                durationSeconds = episodeInfo.duration.toLong()
                ageLimit = episodeInfo.restrictionAge

                episodeInfo.logo?.let { logoUrls = vhConverter.buildImage(it, VhConverter.LOGO_IMAGE_RESOLUTIONS) }
                titleText = episodeInfo.title
                episodeInfo.thumbnailLogos.firstOrNull()?.url?.let {
                    legalLogoUrls = vhConverter.buildImage(it, VhConverter.LEGAL_LOGO_IMAGE_RESOLUTIONS)
                }
                thumbnailUrls = vhConverter.buildImage(episodeInfo.cover, VhConverter.THUMBNAIL_IMAGE_RESOLUTIONS)
                episodeInfo.ratingKp?.let { rating = it.toFloat() }
                episodeInfo.countries?.let { countries = it }
                onVhContentCardOpenedEvent = protoUtils.messageToStruct(
                    vhConverter.createAppmetrikaOpenEvent(
                        episodeInfo,
                        CardContext(carouselContext, ind + 1),
                    )
                )
                carouselItemGeneralAnalyticsInfoPayload = protoUtils.messageToStruct(
                    vhConverter.createAppmetrikaCardEventGeneralPayload(
                        documentContentId = episodeInfo.contentId,
                        documentContentType = episodeInfo.contentTypeName,
                        carouselContext = carouselContext,
                    )
                )
                vhConverter.getSubscriptionTypes(episodeInfo)?.let {
                    addAllSubscriptionTypes(it)
                }

            }.build()

        }

        val galleryData = TGalleryData.newBuilder().apply {
            galleryId = carousel.carouselId
            galleryTitle = carousel.title
            tvContext.parentScreenId?.let { galleryParentScreen = it }
            galleryPosition = 1
            feed.requestInfo?.apphostReqid?.let { apphostRequestId = it }
            requestId = feed.reqid

            addAllCards(cards)
        }

        return TDivRenderData.newBuilder().apply {
            scenarioDataBuilder.setGalleryData(galleryData)
        }.build()
    }

    private fun renderDataForFirstCarousel(ott: ShowcaseGatewayDto, tvContext: TvContext): TDivRenderData {
        val carousel = ott.collections[0]

        val carouselContext = CarouselContext(
            title = carousel.title ?: "Unknown carousel",
            carouselId = carousel.selectionId,
            carouselPosition = 0,
            tvContext = tvContext,
        )

        val cards: List<TCard> = vhConverter.makeOttCarousel(carousel, carouselContext)
        val galleryData = TGalleryData.newBuilder().apply {
            galleryId = carousel.selectionId
            galleryTitle = carousel.title
            tvContext.parentScreenId?.let { galleryParentScreen = it }
            galleryPosition = 1
            apphostRequestId = tvContext.apphostRequestId
            requestId = tvContext.requestId
            addAllCards(cards)
        }

        return TDivRenderData.newBuilder().apply {
            scenarioDataBuilder.setGalleryData(galleryData)
        }.build()
    }

    override fun prepareRun(
        request: MegaMindRequest<Any>,
        args: TGetVideoGalleriesSemanticFrame,
        responseBuilder: ScenePrepareBuilder
    ) {
        val categoryId = args.takeIf { it.hasCategoryId() }
            ?.categoryId
            ?.stringValue?.takeIf { it.isNotEmpty() }
            ?.let { if (it == "main") "tvandroid" else it }

        val relev = args.takeIf { it.hasKidModeEnabled() && it.kidModeEnabled.boolValue && it.hasRestrictionAge() }
            ?.let { "age_limit=${it.restrictionAge.stringValue}" }

        val offset = args.takeIf { it.hasOffset() }?.offset?.numValue ?: 0

        val useOttApi = request.hasExperiment(USE_OTT_API_MAIN)
        val path = if (!useOttApi) {
            UriComponentsBuilder.fromPath("/v23/feed.json")
                .queryParamIfPresent("tag", Optional.ofNullable(categoryId))
                .queryParam("offset", offset)
                .queryParam("limit", args.takeIf { it.hasLimit() }?.limit?.numValue ?: 12)
                .queryParam("num_docs", args.takeIf { it.hasMaxItemsPerGallery() }?.maxItemsPerGallery?.numValue ?: 10)
                .queryParamIfPresent(
                    "cache_hash",
                    Optional.ofNullable(args.takeIf { it.hasCacheHash() }?.cacheHash?.stringValue)
                )
                .queryParamIfPresent("relev", Optional.ofNullable(relev))
                .queryParam("service", "ya-tv-android")
                .queryParam("from", "tvandroid")
                .apply {
                    if (offset == 0) {
                        this.queryParam("is_tvandroid_main", "1")
                    }
                }
                .toUriString()
        } else {
            UriComponentsBuilder.fromPath("")
                .queryParam("selectionsOffset", offset)
                .queryParam("selectionsLimit", args.takeIf { it.hasLimit() }?.limit?.numValue ?: 12)
                .queryParam("selectionWindowId", ottApiSelectionWindowId)
                .queryParam("itemsLimit", args.takeIf { it.hasMaxItemsPerGallery() }?.maxItemsPerGallery?.numValue ?: 10)
                .queryParam("serviceId", ottApiServiceId)
                .toUriString()
        }

        val httpRequest = HttpRequest.builder<Any>(path)
            .headerIfPresent(USER_TICKET_HEADER, requestContext.currentUserTicket)
            .headerIfPresent(HttpHeaders.X_FORWARDED_FOR, requestContext.forwardedFor)
            .headerIfPresent(HttpHeaders.USER_AGENT, requestContext.userAgent)
            .headerIfPresent(HttpHeaders.X_REQUEST_ID, requestContext.requestId)
            .headerIfPresent(HttpHeaders.AUTHORIZATION, "OAuth ${requestContext.oauthToken}")
            .headerIfPresent("X-Device-Id", request.clientInfo.deviceId)
            .build()

        if (!useOttApi) {
            logger.info("Request to feed: {}, {}", path, httpRequest.headers.filterKeys { key -> key != HttpHeaders.AUTHORIZATION && key != USER_TICKET_HEADER })
            responseBuilder.addHttpRequest("feed_request", httpRequest)
        } else {
            logger.info("Request to ott-api selections: {}, {}", path, httpRequest.headers.filterKeys { key -> key != HttpHeaders.AUTHORIZATION && key != USER_TICKET_HEADER })
            responseBuilder.addHttpRequest("ott_selections_request", httpRequest)
        }
    }

    @OptIn(ExperimentalTime::class)
    fun getVhData(
        request: MegaMindRequest<Any>,
        args: TGetVideoGalleriesSemanticFrame,
    ): TvHomeResultProto.TTvFeedResultData {
        val vhHttpResponse = request.additionalSources.getSingleItem<THttpResponse>("feed_response")
        if (vhHttpResponse == null) {
            NO_VH_RESPONSE_ERROR_RATE.inc()
            throw RuntimeException("No VH response")
        }

        logger.debug("Vh returned response with code{}\n{}", vhHttpResponse.statusCode, vhHttpResponse.content.toStringUtf8())

        val feedResponse = request.additionalSources.getSingleHttpResponse<FeedResponse>("feed_response")?.content
            ?: throw RuntimeException("Could not parse vh response")

        val tvContext = TvContext(
            deviceId = request.clientInfo.deviceId!!,
            parentScreenId = args.parentFromScreenId.stringValue.takeIf { it.isNotEmpty() },
            screenId = args.fromScreenId.stringValue,
            offset = args.offset.numValue,
            limit = args.maxItemsPerGallery?.numValue ?: 10,
            requestId = feedResponse.reqid.orEmpty(),
            apphostRequestId = feedResponse.requestInfo?.apphostReqid.orEmpty(),
        )

        val div2Body = if (args.offset.numValue != 0) {
            null
        } else {
            val galleryData = renderDataForFirstCarousel(feedResponse, tvContext).scenarioData.galleryData

            val (card, processingDuration) = measureTimedValue {
                TvMainTemplate.renderGallery(galleryData).toJsonString()
            }
            PROCESSING_DURATION_HIST.record(processingDuration.toLong(DurationUnit.MICROSECONDS))
            card
        }
        return vhConverter.makeFeedResult(feedResponse, tvContext, div2Body)
    }

    @OptIn(ExperimentalTime::class)
    private fun getOttData(
        request: MegaMindRequest<Any>,
        args: TGetVideoGalleriesSemanticFrame
    ): TvHomeResultProto.TTvFeedResultData {
        val httpResponse = request.additionalSources.getSingleItem<THttpResponse>("ott_selections_response")
        if (httpResponse == null) {
            NO_OTT_RESPONSE_ERROR_RATE.inc()
            throw RuntimeException("No OTT response")
        }
        logger.debug("OTT returned response with code {}\n{}", httpResponse.statusCode, httpResponse.content.toStringUtf8())

        val apphostReqId = httpResponse.headersList.find { it.name.equals("X-Response-Request-Id") }?.value.orEmpty()
        val ottResponse = request.additionalSources.getSingleHttpResponse<ShowcaseGatewayDto>("ott_selections_response")?.content
            ?: throw RuntimeException("Could not serialize Ott response: ${httpResponse.content}")


        val tvContext = TvContext(
            deviceId = request.clientInfo.deviceId!!,
            parentScreenId = args.parentFromScreenId.stringValue.takeIf { it.isNotEmpty() },
            screenId = args.fromScreenId.stringValue,
            offset = args.offset.numValue,
            limit = args.maxItemsPerGallery?.numValue ?: 10,
            requestId = ottResponse.pagingMeta?.sessionId.orEmpty(),
            apphostRequestId = apphostReqId,
        )

        val galleryData = renderDataForFirstCarousel(ottResponse, tvContext).scenarioData.galleryData
        val (card, processingDuration) = measureTimedValue {
            TvMainTemplate.renderGallery(galleryData).toJsonString()
        }
        PROCESSING_DURATION_HIST.record(processingDuration.toLong(DurationUnit.MICROSECONDS))

        return vhConverter.makeFeedResult(ottResponse, tvContext, card)
    }

    @OptIn(ExperimentalTime::class)
    override fun render(request: MegaMindRequest<Any>, args: TGetVideoGalleriesSemanticFrame): RelevantResponse<Any> {
        val feedResultData: TvHomeResultProto.TTvFeedResultData = if (request.hasExperiment(USE_OTT_API_MAIN)) {
            getOttData(request, args)
        } else {
            getVhData(request, args)
        }

        return RunOnlyResponse(
            layout = Layout.directiveOnlyLayout(GrpcDirective(feedResultData)),
            state = null,
            analyticsInfo = AnalyticsInfo("get_galleries")
        )
    }

    override val applyArgsClass: KClass<Empty>
        get() = Empty::class

    override fun processContinue(
        request: MegaMindRequest<Any>,
        args: TGetVideoGalleriesSemanticFrame,
        applyArg: Empty
    ): ScenarioResponseBody<Any> {
        TODO("Not yet implemented")
    }

    override fun prepareContinue(
        request: MegaMindRequest<Any>,
        args: TGetVideoGalleriesSemanticFrame,
        applyArg: Empty,
        responseBuilder: ScenePrepareBuilder
    ) {
        TODO("Not yet implemented")
    }

    companion object {
        private val logger = LogManager.getLogger(GetGalleryScene::class.java)
    }
}
