package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.teasers

import org.slf4j.LoggerFactory
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.ActionRef
import ru.yandex.alice.kronstadt.core.AliceHandledException
import ru.yandex.alice.kronstadt.core.BaseRunResponse
import ru.yandex.alice.kronstadt.core.ContinueNeededResponse
import ru.yandex.alice.kronstadt.core.DivRenderData
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoContext
import ru.yandex.alice.kronstadt.core.directive.AddGalleryCardDirective
import ru.yandex.alice.kronstadt.core.domain.converters.TeaserPreviewDataConverter
import ru.yandex.alice.kronstadt.core.domain.scenariodata.teasers.TeaserConfigData
import ru.yandex.alice.kronstadt.core.domain.scenariodata.teasers.TeaserSkillCardData
import ru.yandex.alice.kronstadt.core.domain.scenariodata.teasers.TeasersPreviewData
import ru.yandex.alice.kronstadt.core.layout.Layout
import ru.yandex.alice.paskill.dialogovo.config.TeasersConfig
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType
import ru.yandex.alice.paskill.dialogovo.domain.Experiments
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context
import ru.yandex.alice.paskill.dialogovo.megamind.processor.ContinuingRunProcessor
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillProvider
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoScenarioDataConverter
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType
import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticFrames.ALICE_CENTAUR_COLLECT_CARDS
import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticFrames.ALICE_CENTAUR_COLLECT_TEASERS_PREVIEW
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.ErrorAnalyticsInfoAction
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.applyarguments.TeasersApplyArguments
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.renderer.SkillTeaserRenderService
import ru.yandex.alice.paskill.dialogovo.semanticframes.FixedActivate
import ru.yandex.alice.protos.data.scenario.Data

@Component
class TeasersCollectingProcessor(
    private val teasersConfig: TeasersConfig,
    private val dialogovoScenarioDataConverter: DialogovoScenarioDataConverter,
    private val teaserService: TeaserService,
    private val skillTeaserRenderService: SkillTeaserRenderService,
    private val skillProvider: SkillProvider,
    private val teaserPreviewDataConverter: TeaserPreviewDataConverter,
) : ContinuingRunProcessor<DialogovoState, TeasersApplyArguments> {

    override val type: RunRequestProcessorType = RunRequestProcessorType.TEASERS_COLLECTOR
    override val applyArgsType = TeasersApplyArguments::class.java

    override fun processContinue(
        request: MegaMindRequest<DialogovoState>,
        context: Context,
        applyArguments: TeasersApplyArguments
    ): ScenarioResponseBody<DialogovoState> {

        if (request.hasSemanticFrame(ALICE_CENTAUR_COLLECT_CARDS)) {
            return processTeasers(request, context, applyArguments)
        } else {
            return processTeasersPreviews(request, context, applyArguments)
        }
    }

    override fun canProcess(request: MegaMindRequest<DialogovoState>): Boolean {
        return hasExperiment(Experiments.COLLECT_TEASERS_SKILLS)
            .and(hasFrame(ALICE_CENTAUR_COLLECT_CARDS))
            .or(
                hasExperiment(Experiments.TEASER_SETTINGS).and(hasFrame(ALICE_CENTAUR_COLLECT_TEASERS_PREVIEW))
            )
            .test(request)
    }

    override fun process(context: Context, request: MegaMindRequest<DialogovoState>): BaseRunResponse<DialogovoState> {
        val skillsFromExp = request.getExperimentStartWith(Experiments.SKILLS_FOR_TEASERS_PREFIX)?.let {
            parseSkillsFromExp(it)
        }.orEmpty()
        return ContinueNeededResponse(
            TeasersApplyArguments(
                skillsFromExp.union(teasersConfig.skillsIds).toList()
            )
        )
    }

    private fun processTeasersPreviews(
        request: MegaMindRequest<DialogovoState>,
        context: Context,
        applyArguments: TeasersApplyArguments
    ): ScenarioResponseBody<DialogovoState> {
        val listOfResults: List<TeaserSkillCardData> =
            getTeasersPreviewCardData(request, applyArguments.skillIds, context)
        return ScenarioResponseBody(
            layout = Layout.silence(),
            state = null,
            analyticsInfo = AnalyticsInfo("collect_teasers_preview"),
            scenarioData = Data.TScenarioData.newBuilder().setTeasersPreviewData(
                teaserPreviewDataConverter.convert(
                    TeasersPreviewData(
                        teaserPreviews = listOfResults.map {
                            TeasersPreviewData.TeaserPreview(
                                teaserConfigData = TeaserConfigData(
                                    teaserType = DIALOGOVO_TEASER_TYPE,
                                    teaserId = it.teaserId
                                ),
                                teaserName = "Навык ${it.skillInfo.name}",
                                previewScenarioData = it
                            )
                        }
                    ), ToProtoContext()
                )
            ).build()
        )
    }

    private fun processTeasers(
        request: MegaMindRequest<DialogovoState>,
        context: Context,
        applyArguments: TeasersApplyArguments
    ): ScenarioResponseBody<DialogovoState> {
        val listOfResults: List<TeaserSkillCardData> =
            getTeasersCardData(request, applyArguments.skillIds, context)

        val actions: Map<String, ActionRef> = listOfResults.associate { it.teaserId to getSkillActivateAction(it) }

        return ScenarioResponseBody(
            layout = Layout.directiveOnlyLayout(
                listOfResults.map {
                    AddGalleryCardDirective(
                        it.teaserId,
                        TeaserConfigData(DIALOGOVO_TEASER_TYPE, it.skillInfo.skillId)
                    )
                }
            ),
            state = null,
            analyticsInfo = AnalyticsInfo("collect_teasers"),
            renderData = listOfResults.map {
                DivRenderData(
                    cardId = it.teaserId,
                    scenarioData = dialogovoScenarioDataConverter.convert(it, ToProtoContext())
                )
            },
            actions = actions
        )
    }

    private fun parseSkillsFromExp(exp: String) =
        exp.drop(Experiments.SKILLS_FOR_TEASERS_PREFIX.length).split(",").toSet()

    private fun getTeasersCardData(
        request: MegaMindRequest<DialogovoState>,
        skillIds: List<String>,
        context: Context?,
    ): List<TeaserSkillCardData> {
        val skillInfos = getSkillInfosByIds(skillIds)
        val responses = teaserService.process(request, skillInfos, context)
        val teasersCardDataList = responses.filter { it.teasersItem.isPresent }
            .flatMap { skillProcessResult ->
                val teaserMeta = skillProcessResult.teasersItem.get()
                val skillInfo = skillProcessResult.skill
                return@flatMap teaserMeta.mapIndexed { index, teaser ->
                    skillTeaserRenderService.getTeaserSkillCardData(
                        skillInfo,
                        teaser,
                        "${index}_${skillInfo.id}"
                    )
                }
            }
        return teasersCardDataList.sortedBy { it.teaserId }.take(teasersConfig.maxNumberOfTeasers)
    }

    private fun getTeasersPreviewCardData(
        request: MegaMindRequest<DialogovoState>,
        skillIds: List<String>,
        context: Context?,
    ): List<TeaserSkillCardData> {
        val skillInfos = getSkillInfosByIds(skillIds)
        return teaserService.process(request, skillInfos, context)
            .filter { it.teasersItem.isPresent && it.teasersItem.get().isNotEmpty() }
            .map { skillProcessResult ->
                val skillInfo = skillProcessResult.skill
                skillTeaserRenderService.getTeaserSkillCardData(
                    skillInfo,
                    skillProcessResult.teasersItem.get().first(),
                    skillInfo.id
                )
            }
    }

    private fun getSkillInfoById(skillId: String): SkillInfo {
        return skillProvider.getSkill(skillId).orElseThrow {
            AliceHandledException(
                "Skill not found $skillId",
                ErrorAnalyticsInfoAction.REQUEST_FAILURE, "Навык не найден", "Навык не найден"
            )
        }
    }

    private fun getSkillInfosByIds(skillIds: List<String>): List<SkillInfo> {
        return skillIds.mapNotNull {
            try {
                getSkillInfoById(it)
            } catch (e: Exception) {
                logger.error("Cannot find skill with id $it", e)
                null
            }
        }
    }

    private fun getSkillActivateAction(teaserSkillCardData: TeaserSkillCardData): ActionRef {
        return ActionRef.withTypedSemanticFrame(
            ACTIVATE_SKILL_UTTERANCE,
            FixedActivate(
                activationCommand = teaserSkillCardData.tapAction?.activationCommand,
                payload = teaserSkillCardData.tapAction?.payload,
                activationSourceType = ActivationSourceType.TEASERS,
                skillId = teaserSkillCardData.skillInfo.skillId
            ),
            ActionRef.NluHint()
        )
    }

    companion object {
        private val logger = LoggerFactory.getLogger(TeasersCollectingProcessor::class.java)
        private const val ACTIVATE_SKILL_UTTERANCE = "запусти навык"
        private const val DIALOGOVO_TEASER_TYPE = "Dialogovo";
    }
}
