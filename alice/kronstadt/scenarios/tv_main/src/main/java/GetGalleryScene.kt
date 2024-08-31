package ru.yandex.alice.kronstadt.scenarios.tvmain

import NAppHostHttp.Http
import com.google.common.net.HttpHeaders
import org.apache.logging.log4j.LogManager
import org.springframework.beans.factory.annotation.Value
import org.springframework.stereotype.Component
import org.springframework.web.util.UriComponentsBuilder
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RelevantResponse
import ru.yandex.alice.kronstadt.core.RequestContext
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.ScenePrepareBuilder
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.directive.GrpcDirective
import ru.yandex.alice.kronstadt.core.layout.Layout
import ru.yandex.alice.kronstadt.core.scenario.AbstractScene
import ru.yandex.alice.kronstadt.core.scenario.SceneWithPrepare
import ru.yandex.alice.kronstadt.core.tvm.USER_TICKET_HEADER
import ru.yandex.alice.paskills.common.apphost.http.HttpRequest
import ru.yandex.alice.protos.data.tv.home.TvHomeResultProto
import ru.yandex.alice.protos.data.tv.home.TvHomeResultProto.THomeCarouselWrapper
import ru.yandex.kronstadt.alice.scenarios.tv_main.proto.TVideoGetGallerySceneArgs
import ru.yandex.monlib.metrics.registry.MetricRegistry
import java.util.Optional

@Component
class GetGalleryScene(
    private val requestContext: RequestContext,
    private val vhConverter: VhConverter,
    private val metricRegistry: MetricRegistry,
    @Value("\${tv-main.carousel-page-size}") private val carouselPageSize: Int
) :
    AbstractScene<Any, TVideoGetGallerySceneArgs>("get_gallery", TVideoGetGallerySceneArgs::class),
    SceneWithPrepare<Any, TVideoGetGallerySceneArgs> {

    private var TAG_CAROUSEL_ID = "hhogwwkkxyuausthh"

    private val NO_VH_RESPONSE_ERROR_RATE = metricRegistry.rate("no_vh_response")

    override fun prepareRun(
        request: MegaMindRequest<Any>,
        args: TVideoGetGallerySceneArgs,
        responseBuilder: ScenePrepareBuilder
    ) {
        var selectedTags = args.selectedTagsList
        val uriComponentsBuilder = UriComponentsBuilder.fromPath("/v23/carousel_videohub.json")
            .queryParam("carousel_id", args.id)
            .queryParam("limit", carouselPageSize)
            .queryParamIfPresent("docs_cache_hash", Optional.ofNullable(args.cacheHash))
            .queryParam("service", "ya-tv-android")
            .queryParam("from", "tvandroid")


        if (selectedTags.isNotEmpty() && args.id == TAG_CAROUSEL_ID) {
            uriComponentsBuilder.queryParam("tag", "catalog")
                .queryParamIfPresent("offset", Optional.ofNullable(args.offset))
                .queryParam("pron", "rec_param=new_candidate_generation=1")
            selectedTags.forEach { tag ->
                uriComponentsBuilder.queryParam("pron", "rec_param=" + tagMap[tag.tagTypeValue] + "=" + tag.id)
            }
        }

        val path = uriComponentsBuilder.build().toUriString()
        val vhRequest = HttpRequest.builder<Any>(path)
            .headerIfPresent(USER_TICKET_HEADER, requestContext.currentUserTicket)
            .headerIfPresent(HttpHeaders.X_FORWARDED_FOR, requestContext.forwardedFor)
            .headerIfPresent(HttpHeaders.USER_AGENT, requestContext.userAgent)
            .headerIfPresent(HttpHeaders.X_REQUEST_ID, requestContext.requestId)
            .apply {
            requestContext.oauthToken?.let { this.header(HttpHeaders.AUTHORIZATION, "OAuth $it") }
            }
            .build()
        logger.info("Request to carousel_videohub: {}, {}", path, vhRequest.headers.filterKeys { key -> key != HttpHeaders.AUTHORIZATION && key != USER_TICKET_HEADER })
        responseBuilder.addHttpRequest(type = "carousel_request", payload = vhRequest)
    }

    override fun render(request: MegaMindRequest<Any>, args: TVideoGetGallerySceneArgs): RelevantResponse<Any> {
        val httpResponse = request.additionalSources.getSingleHttpResponse<VhCarousel>("carousel_response")
        if (httpResponse == null) {
            NO_VH_RESPONSE_ERROR_RATE.inc()
            throw RuntimeException("No vh response")
        }
        if (!httpResponse.is2xxSuccessful) {
            throw RuntimeException("vh request failed with code: ${httpResponse.statusCode}")
        }

        if (logger.isDebugEnabled) {
            logger.debug("Vh response\n${request.additionalSources.getSingleItem<Http.THttpResponse>("carousel_response")?.content?.toStringUtf8()}")

            logger.debug("Parsed Vh response\n${httpResponse.content}")
        }

        val body = httpResponse.content!!

        val tvContext = TvContext(
            deviceId = request.clientInfo.deviceId!!,
            parentScreenId = args.parentFromScreenId?.takeIf { it.isNotEmpty() },
            screenId = "main"/*args.fromScreenId*/,
            offset = args.offset,
            limit = carouselPageSize,
            requestId = body.reqid.orEmpty(),
            apphostRequestId = body.requestInfo?.apphostReqid.orEmpty(),
        )

        val ctx = CarouselContext(args.carouselTitle, args.id, args.carouselPosition, tvContext)

        val homeCarouselWrapper: THomeCarouselWrapper = vhConverter.makeCarousel(body, ctx)
        if (homeCarouselWrapper.homeCarousel.itemsList.isEmpty()) {
            logger.warn("Empty gallery")
            throw RuntimeException("Empty gallery")
        }

        return RunOnlyResponse(
            layout = Layout.directiveOnlyLayout(
                GrpcDirective(
                    TvHomeResultProto.TTvCarouselResultData.newBuilder()
                        .setCarousel(homeCarouselWrapper)
                        .build()
                )
            ),
            state = null,
            analyticsInfo = AnalyticsInfo("get_gallery")
        )
    }

    companion object {
        private val logger = LogManager.getLogger(GetGalleryScene::class.java)
        private var tagMap = mapOf(
            1 to "content_type",
            2 to "genre",
            3 to "country",
            4 to "year",
            6 to "quality",
            7 to "sort_order"
        )
    }
}
