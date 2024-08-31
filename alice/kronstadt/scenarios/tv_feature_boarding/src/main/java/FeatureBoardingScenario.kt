package ru.yandex.alice.kronstadt.scenarios.tv_feature_boarding

import org.apache.logging.log4j.LogManager
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.ScenarioMeta
import ru.yandex.alice.kronstadt.core.scenario.AbstractNoStateScenario
import ru.yandex.alice.kronstadt.core.scenario.SelectedScene
import ru.yandex.alice.kronstadt.scenarios.tv_feature_boarding.scene.ReportPromoShownScene
import ru.yandex.alice.kronstadt.scenarios.tv_feature_boarding.scene.TandemPromoScene
import ru.yandex.alice.kronstadt.scenarios.tv_feature_boarding.scene.args.ReportPromoShownArgs
import ru.yandex.alice.kronstadt.scenarios.tv_feature_boarding.scene.args.ShowCounts
import ru.yandex.alice.kronstadt.scenarios.tv_feature_boarding.scene.args.TandemPromoArgs
import ru.yandex.alice.megamind.protos.common.FrameProto.TTvChosenTemplateSlot
import ru.yandex.alice.protos.data.tv_feature_boarding.TvFeatureBoardingTemplate
import ru.yandex.alice.protos.data.tv_feature_boarding.TvFeatureBoardingTemplate.TTandemTemplate

val FEATUREBOARDING_SCENARIO =
    ScenarioMeta("tv_feature_boarding", "TvFeatureBoarding", "tv_feature_boarding", mmPath = "tv_feature_boarding")

// Frames
const val GET_PROMO_FRAME = "alice.tv.get_promo_template"
const val REPORT_PROMO_FRAME = "alice.tv.report_promo_template_shown"

@Component
class FeatureBoardingScenario() :
    AbstractNoStateScenario(scenarioMeta = FEATUREBOARDING_SCENARIO) {

    private val logger = LogManager.getLogger(FeatureBoardingScenario::class.java)

    override fun selectScene(request: MegaMindRequest<Any>): SelectedScene.Running<Any, *>? = request.handle {
        onFrame(GET_PROMO_FRAME) { frame ->
            frame.typedSemanticFrame?.let {
                val tsf = it.tvPromoTemplateRequestSemanticFrame
                val chosenTemplateCase = tsf.chosenTemplate.valueCase
                onCondition({ chosenTemplateCase == TTvChosenTemplateSlot.ValueCase.TANDEMTEMPLATE }) {
                    val tandemTemplate = tsf.chosenTemplate.tandemTemplate
                    val templateName = getPromoTemplateName(tandemTemplate)
                    val lastAppearanceTime =
                        request.mementoData.userConfigs.tandemPromoTemplateInfo.lastAppearanceTime.toULong()
                    val showCount = request.mementoData.userConfigs.tandemPromoTemplateInfo.showCount.toUInt()
                    sceneWithArgs(
                        TandemPromoScene::class,
                        TandemPromoArgs(
                            tandemTemplate.isTandemDevicesAvailable,
                            tandemTemplate.isTandemConnected,
                            templateName,
                            lastAppearanceTime,
                            showCount
                        )
                    )
                }
            }
        }
        onFrame(REPORT_PROMO_FRAME) { frame ->
            frame.typedSemanticFrame?.let {
                val tsf = it.tvPromoTemplateShownReportSemanticFrame
                val chosenTemplateCase = tsf.chosenTemplate.valueCase
                onCondition({ chosenTemplateCase != TTvChosenTemplateSlot.ValueCase.VALUE_NOT_SET }) {
                    val chosenTemplate = when (chosenTemplateCase) {
                        TTvChosenTemplateSlot.ValueCase.TANDEMTEMPLATE -> tsf.chosenTemplate.tandemTemplate
                        else -> throw RuntimeException("Unknown template case")
                    }
                    sceneWithArgs(
                        ReportPromoShownScene::class,
                        ReportPromoShownArgs(
                            chosenTemplateCase,
                            getPromoTemplateName(chosenTemplate),
                            request.serverTime.toEpochMilli(),
                            ShowCounts(
                                tandemShowCount = request.mementoData.userConfigs.tandemPromoTemplateInfo.showCount
                            )
                        )
                    )
                }
            }
        }
    }

    /**
     * Extracts option value:
     * https://a.yandex-team.ru/arc/trunk/arcadia/alice/protos/data/tv_feature_boarding/template.proto?rev=r9064716#L14
     */
    private fun getPromoTemplateName(chosenTemplate: TTandemTemplate): String {
        return chosenTemplate.descriptorForType.options.getExtension(TvFeatureBoardingTemplate.promoTemplateName)
    }
}


