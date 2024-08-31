package ru.yandex.alice.kronstadt.scenarios.discovery.implicit

import org.apache.logging.log4j.LogManager
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RelevantResponse
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoAction
import ru.yandex.alice.kronstadt.core.analytics.analyticsInfo
import ru.yandex.alice.kronstadt.core.layout.Button.Companion.simpleTextWithTextDirective
import ru.yandex.alice.kronstadt.core.layout.Layout
import ru.yandex.alice.kronstadt.core.layout.TextWithTts
import ru.yandex.alice.kronstadt.core.scenario.AbstractScene
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrame
import ru.yandex.alice.kronstadt.core.text.Phrases
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillProvider
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillTagsKey
import ru.yandex.alice.paskill.dialogovo.scenarios.SkillFilters
import ru.yandex.alice.paskill.dialogovo.scenarios.SkillsSuggestVoiceButtonFactory
import ru.yandex.alice.paskill.dialogovo.scenarios.analytics.SkillAnalyticsInfoObject
import ru.yandex.alice.paskill.dialogovo.service.SurfaceChecker

private const val MAX_IMPLICIT_SKILL_RECOMMENDATIONS_SIZE = 5

@Component
open class ImplicitSkillDiscoveryStationScene(
    private val skillProvider: SkillProvider,
    private val phrases: Phrases,
    private val voiceButtonFactory: SkillsSuggestVoiceButtonFactory,
    private val surfaceChecker: SurfaceChecker,
) : AbstractScene<Any, SkillInfo>("IMPLICIT_SKILL_DISCOVERY_STATION", SkillInfo::class) {

    private val logger = LogManager.getLogger()

    override fun render(request: MegaMindRequest<Any>, skillInfo: SkillInfo): RelevantResponse<Any> {

        val nlg = phrases.getRandomTextWithTts(
            "station.discovery.implicit.suggest",
            request.random,
            TextWithTts(skillInfo.name, skillInfo.getNameTts())
        )
        return RunOnlyResponse(
            layout = Layout.textLayout(
                text = nlg.text,
                outputSpeech = nlg.tts,
                shouldListen = true,
                suggests = listOf(
                    simpleTextWithTextDirective(phrases["suggest.confirm.yes"]),
                    simpleTextWithTextDirective(phrases["suggest.confirm.no"])
                )
            ),
            state = null,
            analyticsInfo = analyticsInfo(intent = "implicit_skill_discovery.suggest") {
                action(skillSuggestAction(skillInfo))
                obj(SkillAnalyticsInfoObject(skillInfo))
            },
            actions = mapOf(
                "confirm" to
                    voiceButtonFactory.createConfirmSkillActivationVoiceButton(
                        skillInfo,
                        ActivationSourceType.DISCOVERY,
                        findFirstActivationSemanticFrame(request.input.semanticFrames)
                    ),
                "decline" to voiceButtonFactory.createDeclineDoNothingButton()
            )
        )
    }

    fun getImplicitRecommendationSkills(
        request: MegaMindRequest<Any>,
        tags: SkillTagsKey
    ): List<SkillInfo> =
        skillProvider.getSkillsByTags(tags)
            .filterValidForRecommendations(request).take(MAX_IMPLICIT_SKILL_RECOMMENDATIONS_SIZE)
            .also { logger.info("Discovered {} implicit recommendation skills: {}", it.size, it.joinToString(", ")) }

    private fun List<SkillInfo>.filterValidForRecommendations(request: MegaMindRequest<Any>) =
        filter { skillInfo ->
            (SkillFilters.VALID_FOR_RECOMMENDATIONS.test(skillInfo) ||
                // If skill is private, but user has access to it, we can recommend it
                (skillInfo.skillAccess == SkillInfo.SkillAccess.PRIVATE &&
                    skillInfo.isAccessibleBy(request.userId, request))) &&
                surfaceChecker.isSkillSupported(request.clientInfo, skillInfo.surfaces) &&
                SkillFilters.EXPLICIT_CONTENT_RECOMMENDATION_FILTER.test(request, skillInfo)
        }

    private fun skillSuggestAction(skill: SkillInfo) = AnalyticsInfoAction(
        "implicit_skill_discovery.suggest",
        "implicit_skill_discovery.suggest",
        "Предложение запустить навык «${skill.name}»"
    )

    companion object {
        @JvmStatic
        fun findFirstActivationSemanticFrame(frames: List<SemanticFrame>) =
            frames.find { it.typedSemanticFrame?.hasPutMoneyOnPhoneSemanticFrame() == true }?.typedSemanticFrame
                ?: throw RuntimeException("Can't find activation semantic frame in request")
    }
}
