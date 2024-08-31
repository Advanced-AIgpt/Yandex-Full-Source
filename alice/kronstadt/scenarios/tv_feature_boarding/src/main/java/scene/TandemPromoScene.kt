package ru.yandex.alice.kronstadt.scenarios.tv_feature_boarding.scene

import com.google.protobuf.Struct
import org.apache.logging.log4j.LogManager
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RelevantResponse
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoAction
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoObject
import ru.yandex.alice.kronstadt.core.analytics.analyticsInfo
import ru.yandex.alice.kronstadt.core.directive.GrpcDirective
import ru.yandex.alice.kronstadt.core.directive.server.MementoChangeUserObjectsDirective
import ru.yandex.alice.kronstadt.core.directive.server.UserConfig
import ru.yandex.alice.kronstadt.core.layout.Layout
import ru.yandex.alice.kronstadt.core.scenario.AbstractScene
import ru.yandex.alice.kronstadt.scenarios.tv_feature_boarding.FeatureBoardingTemplatesProvider
import ru.yandex.alice.kronstadt.scenarios.tv_feature_boarding.scene.args.TandemPromoArgs
import ru.yandex.alice.memento.proto.MementoApiProto
import ru.yandex.alice.memento.proto.UserConfigsProto
import ru.yandex.alice.protos.data.tv_feature_boarding.TvFeatureBoardingTemplate

@Component
class TandemPromoScene(private val templatesProvider: FeatureBoardingTemplatesProvider) :
    AbstractScene<Any, TandemPromoArgs>("tandem_promo_scene", TandemPromoArgs::class) {

    private val logger = LogManager.getLogger()

    data class RenderResult(
        val divJson: Struct?,
        val analyticsInfoObj: AnalyticsInfoObject?
    )

    private val NO_TEMPLATE_SHOWN_NO_SERVER_TIME = AnalyticsInfoObject("template_not_shown", "no_server_time", "Template not shown since no server time provided")
    private val NO_TEMPLATE_SHOWN_TOO_MUCH_REQUESTS_PER_TTL = AnalyticsInfoObject("template_not_shown", "too_much_requests", "Template not shown because of requests are too often")
    private val NO_TEMPLATE_SHOWN_MAX_SHOW_COUNT = AnalyticsInfoObject("template_not_shown", "max_show_count", "Template not shown since max show count exceed")
    private val NO_TEMPLATE_SHOWN_ALREADY_CONNECTED = AnalyticsInfoObject("template_not_shown", "tandem_already_connected", "Template not shown since devices already in tandem")
    private val TEMPLATE_SHOWN_NO_TANDEM_DEVICES = AnalyticsInfoObject("template_shown", "no_tandem_devices", "Template shown with no tandem devices")
    private val TEMPLATE_SHOWN_TANDEM_DEVICES_AVAILABLE = AnalyticsInfoObject("template_shown", "tandem_devices_available", "Template shown with tandem devices available")
    private val SHOW_TANDEM_TEMPLATE_ACTION = AnalyticsInfoAction("tv_feature_boarding.tv.show_tandem_template", "tv_feature_boarding.tv.show_tandem_template","Показ баннера про возможность объединения ТВ и станции в Тандем")

    val DEFAULT_TTL = 60 * 60 * 24 * 7  // 1 week
    val MAX_SHOW_COUNT = 2u

    override fun render(request: MegaMindRequest<Any>, args: TandemPromoArgs): RelevantResponse<Any>? {
        val epochTime = request.serverTime.toEpochMilli().toULong()
        val renderResult = createRenderResult(epochTime, args, request.clientInfo.osVersion)
        val grpcDirective = GrpcDirective(response = TvFeatureBoardingTemplate.TTemplateResponse.newBuilder().apply {
            ttl = if (request.hasExperiment("zero_fb_ttl")) 0 else DEFAULT_TTL
            templateName = args.templateName
            renderResult.divJson?.let { divJson = it }
        }.build())
        val showingTemplate = (renderResult.divJson != null)
        return RunOnlyResponse(
            layout = Layout(outputSpeech = null, shouldListen = false, directives = listOf(grpcDirective)),
            analyticsInfo = analyticsInfo(intent = "tv.feature_boarding.get") {
                if (showingTemplate) action(SHOW_TANDEM_TEMPLATE_ACTION)
                obj(AnalyticsInfoObject("requested_template", args.templateName, "Запрошенный промо"))
                renderResult.analyticsInfoObj?.let { obj(it) }
            },
            state = null,
            serverDirectives = if (showingTemplate) {
                listOf(
                    MementoChangeUserObjectsDirective(mapOf(
                        MementoApiProto.EConfigKey.CK_TANDEM_PROMO_TEMPLATE_INFO to UserConfig(
                            UserConfigsProto.TSmartTvTemplateInfo.newBuilder().apply {
                                lastAppearanceTime = request.serverTime.toEpochMilli()
                                showCount = (args.showCount + 1u).toInt()
                            }.build())
                    ))
                )
            } else listOf()
        )
    }

    private fun createRenderResult(epochTime: ULong, args: TandemPromoArgs, version: String): RenderResult {
        if (epochTime == 0UL) {
            logger.debug("No server time provided")
            return RenderResult(null, NO_TEMPLATE_SHOWN_NO_SERVER_TIME)
        }
        if (epochTime - args.lastShowTime <= DEFAULT_TTL.toULong()) {
            logger.debug("Too much requests per ttl")
            return RenderResult(null, NO_TEMPLATE_SHOWN_TOO_MUCH_REQUESTS_PER_TTL)
        }
        if (args.showCount >= MAX_SHOW_COUNT) {
            logger.debug("Promo template show count limit exceed")
            return RenderResult(null, NO_TEMPLATE_SHOWN_MAX_SHOW_COUNT)
        }
        if (args.isTandemConnected) {
            logger.debug("Tandem is already connected")
            return RenderResult(null, NO_TEMPLATE_SHOWN_ALREADY_CONNECTED)
        }

        val analyticsInfoObject: AnalyticsInfoObject
        val resultStruct: Struct
        if (args.isTandemDevicesAvailable) {
            resultStruct = templatesProvider.getTandemDeviceAvailablePromo(version)
            analyticsInfoObject = TEMPLATE_SHOWN_TANDEM_DEVICES_AVAILABLE
        } else {
            resultStruct = templatesProvider.noTandemDevicePromo
            analyticsInfoObject = TEMPLATE_SHOWN_NO_TANDEM_DEVICES
        }
        return RenderResult(resultStruct, analyticsInfoObject)
    }
}
