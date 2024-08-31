package ru.yandex.alice.kronstadt.scenarios.tv_feature_boarding.scene

import com.google.protobuf.Empty
import org.apache.logging.log4j.LogManager
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RelevantResponse
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoObject
import ru.yandex.alice.kronstadt.core.analytics.analyticsInfo
import ru.yandex.alice.kronstadt.core.directive.GrpcDirective
import ru.yandex.alice.kronstadt.core.directive.server.MementoChangeUserObjectsDirective
import ru.yandex.alice.kronstadt.core.directive.server.ServerDirective
import ru.yandex.alice.kronstadt.core.directive.server.UserConfig
import ru.yandex.alice.kronstadt.core.layout.Layout
import ru.yandex.alice.kronstadt.core.scenario.AbstractScene
import ru.yandex.alice.kronstadt.scenarios.tv_feature_boarding.scene.args.ReportPromoShownArgs
import ru.yandex.alice.megamind.protos.common.FrameProto.TTvChosenTemplateSlot
import ru.yandex.alice.memento.proto.MementoApiProto.EConfigKey
import ru.yandex.alice.memento.proto.UserConfigsProto.TSmartTvTemplateInfo

@Component
class ReportPromoShownScene :
    AbstractScene<Any, ReportPromoShownArgs>("report_promo_shown_scene", ReportPromoShownArgs::class) {

    private val logger = LogManager.getLogger(ReportPromoShownScene::class.java)

    data class PromoMementoData(
        val configKey: EConfigKey,
        val shownCount: Int
    )

    override fun render(request: MegaMindRequest<Any>, args: ReportPromoShownArgs): RelevantResponse<Any>? {
        logger.debug("Reported '{}' template show", args.chosenTemplateCase.name)
        return RunOnlyResponse(
            layout = Layout(outputSpeech = null, shouldListen = false, directives = listOf(getGrpcResponse())),
            analyticsInfo = analyticsInfo(intent = "tv.feature_boarding.report") {
                obj(AnalyticsInfoObject("reported_template", args.chosenTemplateName, "Промо который был показан"))
            },
            state = null,
            serverDirectives = listOf(getMementoChangeUserObjectServerDirective(args))
        )
    }

    private fun getGrpcResponse(): GrpcDirective {
        // Don't need response for this request
        return GrpcDirective(response = Empty.getDefaultInstance())
    }

    private fun getMementoChangeUserObjectServerDirective(args: ReportPromoShownArgs): ServerDirective {
        val promoMementoData = getPromoMementoData(args)
        return MementoChangeUserObjectsDirective(
            userConfigs = mapOf(
                promoMementoData.configKey to UserConfig(
                    TSmartTvTemplateInfo.newBuilder().apply {
                        lastAppearanceTime = args.showTemplateTimestamp
                        showCount = promoMementoData.shownCount + 1
                    }.build()
                )
            )
        )
    }

    private fun getPromoMementoData(args: ReportPromoShownArgs): PromoMementoData {
        return when(args.chosenTemplateCase) {
            TTvChosenTemplateSlot.ValueCase.TANDEMTEMPLATE -> PromoMementoData(
                EConfigKey.CK_TANDEM_PROMO_TEMPLATE_INFO,
                args.showCounts.tandemShowCount
            )
            else -> throw RuntimeException("Chosen unknown template")
        }
    }
}
