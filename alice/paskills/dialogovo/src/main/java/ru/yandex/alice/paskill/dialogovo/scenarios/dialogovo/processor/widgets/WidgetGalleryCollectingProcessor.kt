package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.widgets

import org.apache.logging.log4j.LogManager
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.ActionRef
import ru.yandex.alice.kronstadt.core.AliceHandledException
import ru.yandex.alice.kronstadt.core.BaseRunResponse
import ru.yandex.alice.kronstadt.core.ContinueNeededResponse
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoContext
import ru.yandex.alice.kronstadt.core.domain.scenariodata.ScenarioData
import ru.yandex.alice.kronstadt.core.layout.Layout
import ru.yandex.alice.paskill.dialogovo.config.WidgetsConfig
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType
import ru.yandex.alice.paskill.dialogovo.domain.Experiments
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context
import ru.yandex.alice.paskill.dialogovo.megamind.processor.ContinuingRunProcessor
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillProvider
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState
import ru.yandex.alice.paskill.dialogovo.scenarios.Intents.EXTERNAL_SKILL_COLLECT_WIDGET_GALLERY
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType
import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticFrames
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.ErrorAnalyticsInfoAction
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.applyarguments.WidgetGalleryApplyArguments
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.converters.WidgetGalleryCardDataConverter
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.domain.MainScreenSkillCardData
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.domain.SkillInfoData
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.domain.WidgetGallerySkillCardData
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.renderer.SkillWidgetRendererService
import ru.yandex.alice.paskill.dialogovo.semanticframes.FixedActivate
import ru.yandex.alice.protos.data.scenario.Data.TScenarioData
import ru.yandex.alice.protos.data.scenario.centaur.MainScreen

@Component
class WidgetGalleryCollectingProcessor(
    private val widgetGalleryCardDataConverter: WidgetGalleryCardDataConverter,
    private val skillProvider: SkillProvider,
    private val widgetGalleryService: WidgetGalleryService,
    private val skillWidgetRendererService: SkillWidgetRendererService,
    private val widgetsConfig: WidgetsConfig
) : ContinuingRunProcessor<DialogovoState, WidgetGalleryApplyArguments> {

    override val applyArgsType = WidgetGalleryApplyArguments::class.java

    override fun canProcess(request: MegaMindRequest<DialogovoState>): Boolean {
        return hasExperiment(Experiments.COLLECT_WIDGET_GALLERY_SKILLS)
            .and(hasAnyOfFrames(SemanticFrames.ALICE_CENTAUR_COLLECT_MAIN_SCREEN)).test(request)
    }

    override val type: RunRequestProcessorType = RunRequestProcessorType.WIDGET_GALLERY_COLLECTOR

    override fun process(context: Context, request: MegaMindRequest<DialogovoState>): BaseRunResponse<DialogovoState> {
        val skillsFromExp = request.getExperimentStartWith(Experiments.SKILLS_FOR_WINDGET_PREFIX)?.let {
            parseSkillsFromExp(it)
        }.orEmpty()
        return ContinueNeededResponse<DialogovoState>(
            WidgetGalleryApplyArguments(
                skillsFromExp.union(widgetsConfig.skillsIds).toList()
            )
        )
    }

    override fun processContinue(
        request: MegaMindRequest<DialogovoState>, context: Context, applyArguments: WidgetGalleryApplyArguments
    ): ScenarioResponseBody<DialogovoState> {
        val hasWidgetSlot = request.getSemanticFrame(SemanticFrames.ALICE_CENTAUR_COLLECT_MAIN_SCREEN)!!.hasValuedSlot(WIDGET_SLOT)
        val scenarioDataList: List<ScenarioData> =
            if (hasWidgetSlot) {
                logger.debug("Collecting skills info for widget gallery")
                applyArguments.skillIds.mapNotNull {
                    try {
                        val skillInfo = getSkillInfoById(it)
                        WidgetGallerySkillCardData(
                            SkillInfoData(
                                name = skillInfo.name,
                                logo = skillInfo.logoUrl ?: "",
                                skillId = it
                            )
                        )
                    } catch (e: AliceHandledException) {
                        logger.error("No skill with id $it", e.fillInStackTrace())
                        null
                    }
                }
            } else {
                logger.debug("Collecting skills info for main screen")
                getSkillResponse(request, context, applyArguments.skillIds.mapNotNull {
                    try {
                        getSkillInfoById(it)
                    } catch (e: AliceHandledException) {
                        logger.error("No skill with id $it", e.fillInStackTrace())
                        null
                    }
                })
            }
        val actions: Map<String, ActionRef> =
            if (hasWidgetSlot) {
                emptyMap()
            } else {
                scenarioDataList.map { it as MainScreenSkillCardData }.associate {
                    it.skillInfoData.skillId to getSkillActivateAction(it)
                }
            }

        return ScenarioResponseBody(
            layout = Layout.silence(),
            state = null,
            analyticsInfo = AnalyticsInfo(intent = EXTERNAL_SKILL_COLLECT_WIDGET_GALLERY),
            scenarioData = TScenarioData.newBuilder()
                .setCentaurScenarioWidgetData(
                    MainScreen.TCentaurScenarioWidgetData.newBuilder()
                        .setWidgetType(WIDGET_TYPE)
                        .addAllWidgetCards(scenarioDataList.map {
                            widgetGalleryCardDataConverter.convert(it, ToProtoContext())
                        })
                ).build(),
            actions = actions
        )
    }

    private fun getSkillInfoById(skillId: String): SkillInfo {
        return skillProvider.getSkill(skillId).orElseThrow {
            AliceHandledException(
                "Skill not found $skillId",
                ErrorAnalyticsInfoAction.REQUEST_FAILURE, "Навык не найден", "Навык не найден"
            )
        }
    }

    private fun getSkillResponse(
        request: MegaMindRequest<DialogovoState>,
        context: Context?,
        skills: List<SkillInfo>
    ): List<ScenarioData> {
        val skillProcessResults = widgetGalleryService.process(request, context, skills)
        return skillProcessResults.map { skillWidgetRendererService.getMainScreenCardData(it) }
    }

    private fun getSkillActivateAction(mainScreenSkillCardData: MainScreenSkillCardData): ActionRef {
        return ActionRef.withTypedSemanticFrame(
            " ",
            FixedActivate(
                skillId = mainScreenSkillCardData.skillInfoData.skillId,
                payload = mainScreenSkillCardData.skillResponse?.tapAction?.payload,
                activationCommand = mainScreenSkillCardData.skillResponse?.tapAction?.activationCommand,
                activationSourceType = ActivationSourceType.WIDGET_GALLERY
            ),
            ActionRef.NluHint()
        )
    }

    private fun parseSkillsFromExp(exp: String): Set<String> {
        return exp.drop(Experiments.SKILLS_FOR_WINDGET_PREFIX.length).split(",").toSet()
    }

    companion object {
        private const val WIDGET_TYPE = "external_skill";
        private const val WIDGET_SLOT = "widget_gallery_position"
        private val logger = LogManager.getLogger(WidgetGalleryCollectingProcessor::class.java)
    }
}
