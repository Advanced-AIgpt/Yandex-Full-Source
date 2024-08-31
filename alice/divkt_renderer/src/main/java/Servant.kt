package ru.yandex.alice.divktrenderer

import com.fasterxml.jackson.databind.ObjectMapper
import com.yandex.div.dsl.serializer.toJsonNode
import org.apache.logging.log4j.LogManager
import org.springframework.stereotype.Controller
import ru.yandex.alice.divktrenderer.grpc.DivktRendererGrpcServiceBase
import ru.yandex.alice.divktrenderer.grpc.TDivktRendererRequest
import ru.yandex.alice.divktrenderer.grpc.TDivktRendererResponse
import ru.yandex.alice.divktrenderer.projects.GlobalTemplateRegistry
import ru.yandex.alice.library.protobufutils.JacksonToProtoConverter
import ru.yandex.alice.protos.api.renderer.Api
import ru.yandex.alice.protos.data.scenario.Data
import ru.yandex.monlib.metrics.histogram.Histograms
import ru.yandex.monlib.metrics.labels.Labels
import ru.yandex.monlib.metrics.registry.MetricRegistry
import ru.yandex.web.apphost.api.request.RequestMeta
import java.util.function.Supplier
import kotlin.time.DurationUnit
import kotlin.time.ExperimentalTime
import kotlin.time.measureTimedValue

@OptIn(ExperimentalTime::class)
@Controller
class Servant(
    objectMapper: ObjectMapper,
    private val metricRegistry: MetricRegistry,
    private val templateRegistry: GlobalTemplateRegistry,
) : DivktRendererGrpcServiceBase() {
    private val jacksonToProtoConverter = JacksonToProtoConverter(objectMapper)

    private val logger = LogManager.getLogger()
    private val DURATION_HISTOGRAMS_SUPPLIER = Supplier { Histograms.exponential(22, 1.5, 75.0) }

    private val TOTAL_RENDERING_DURATION = metricRegistry.histogramRate(
        "divkt-renderer.rendering.duration",
        DURATION_HISTOGRAMS_SUPPLIER
    )
    private val TOTAL_RENDERING_REQUESTS = metricRegistry.rate("divkt-renderer.rendering.requests")
    private val TOTAL_RENDERING_FAILURES = metricRegistry.rate("divkt-renderer.rendering.failures")

    override fun render(requestMeta: RequestMeta, request: TDivktRendererRequest): TDivktRendererResponse {
        TOTAL_RENDERING_REQUESTS.inc()

        val (response, duration) = measureTimedValue {
            try {
                val responseBuilder = TDivktRendererResponse.newBuilder()

                val results = request.renderDataList.mapNotNull { renderItem(it, request) }
                responseBuilder.addAllRenderResult(results)

                return@measureTimedValue responseBuilder.build()
            } catch (e: Exception) {
                TOTAL_RENDERING_FAILURES.inc()
                throw e
            }
        }
        TOTAL_RENDERING_DURATION.record(duration.toLong(DurationUnit.MILLISECONDS))

        logger.info("Rendering request finished successfully in $duration")
        logger.debug(response)
        return response
    }

    private fun renderItem(item: Api.TDivRenderData, request: TDivktRendererRequest): Api.TRenderResponse? {
        logger.debug("Incoming request: {}", item)
        val scenarioData = item.scenarioData
        val templateId = scenarioData.dataCase

        if (templateId == Data.TScenarioData.DataCase.DATA_NOT_SET) {
            logger.warn("Invalid proto input: $scenarioData")
            return null
        }

        val templateLabels = Labels.of("template_id", templateId.name)
        metricRegistry.rate("divkt-renderer.templates.processing.requests", templateLabels).inc()

        val (result, duration) = measureTimedValue {
            try {
                val template = templateRegistry.selectTemplateAndRender(scenarioData, request)
                if (template == null) {
                    logger.warn("No template defined for proto template id: $item")
                    return null
                }
                template
            } catch (e: Exception) {
                metricRegistry.rate(
                    "divkt-renderer.templates.processing.failures",
                    templateLabels
                ).inc()
                logger.error("Failed to render $templateId: $e")
                throw e
            }
        }
        metricRegistry.histogramRate(
            "divkt-renderer.templates.processing.duration",
            templateLabels,
            DURATION_HISTOGRAMS_SUPPLIER
        ).record(duration.toLong(DurationUnit.MILLISECONDS))

        logger.debug("Rendering template {} finished successfully in {}", templateId, duration)


        val (protoResult, encodingDuration) = measureTimedValue {
            runCatching {
                Api.TRenderResponse.newBuilder()
                    .setCardId(item.cardId)
                    .setDiv2Body(jacksonToProtoConverter.objectNodeToStruct(result.toJsonNode()))
//                .putAllGlobalDiv2Templates(getTemplates(requestState.globalTemplates, templatesRegistry))
                    .setCardName(result.card.toJsonNode().get("log_id")?.asText() ?: "")
                    .build()
            }.onFailure {
                metricRegistry.rate(
                    "divkt-renderer.templates.encoding.failures",
                    templateLabels
                ).inc()
            }
        }

        logger.debug("Encoding template {} finished successfully in {}", templateId, encodingDuration)

        metricRegistry.histogramRate(
            "divkt-renderer.templates.encoding.duration",
            templateLabels,
            DURATION_HISTOGRAMS_SUPPLIER
        ).record(encodingDuration.toLong(DurationUnit.MILLISECONDS))

        logger.info("Processing template $templateId finished successfully")

        return protoResult.getOrNull()
    }
}
