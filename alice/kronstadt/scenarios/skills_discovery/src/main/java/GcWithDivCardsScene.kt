package ru.yandex.alice.kronstadt.scenarios.skills_discovery

import org.apache.logging.log4j.LogManager
import org.springframework.beans.factory.annotation.Value
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RelevantResponse
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.domain.SkillDiscoveryCandidate
import ru.yandex.alice.kronstadt.core.layout.Layout
import ru.yandex.alice.kronstadt.core.layout.TextCard
import ru.yandex.alice.kronstadt.core.scenario.AbstractScene
import ru.yandex.alice.kronstadt.core.text.Phrases
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType
import ru.yandex.alice.paskill.dialogovo.domain.RecommendationType
import ru.yandex.alice.paskill.dialogovo.domain.SkillRecommendation
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillProvider
import ru.yandex.alice.paskill.dialogovo.scenarios.RecommendationCardsRenderer
import ru.yandex.alice.paskill.dialogovo.scenarios.SkillFilters
import ru.yandex.alice.paskill.dialogovo.scenarios.analytics.SkillRecommendationsAnalyticsInfoObject
import ru.yandex.alice.paskill.dialogovo.scenarios.discovery.SkillRecommendationMapper

private const val MAX_RECOMMENDATIONS_SIZE = 5

@Component
open class GcWithDivCardsScene(
    private val recommendationCardsRenderer: RecommendationCardsRenderer,
    private val skillProvider: SkillProvider,
    private val phrases: Phrases,
    private val skillRecommendationMapper: SkillRecommendationMapper,
    @Value("\${storeUrl}")
    private val storeUrl: String,
) : AbstractScene<Any, GcWithDivCardsScene.Args>(
    "SKILLS_DISCOVERY_GC_WITH_DIV_CARDS",
    argsClass = Args::class
) {

    class Args(val items: List<SkillRecommendation>)

    private val logger = LogManager.getLogger()

    override fun render(
        request: MegaMindRequest<Any>,
        recommendations: Args,
    ): RelevantResponse<Any> {

        val skillsRendererResult = recommendationCardsRenderer.renderSkillRecommendationCardClassic(
            RecommendationCardsRenderer.RecommendationCardsRendererContext.builder()
                .items(recommendations.items)
                .activationSourceType(ActivationSourceType.DISCOVERY)
                .storeUrl(storeUrl)
                .requestId(request.requestId)
                .recommendationSource("mm_discovery_saas_datasource")
                .recommendationType(RecommendationType.SKILLS_DISCOVERY_GC)
                .recommendationSubType("discovery_megamind_gc")
                .build()
        )
        return RunOnlyResponse(
            layout = Layout(
                outputSpeech = phrases["divs.discovery.recommendation.message"],
                shouldListen = true,
                cards = listOf(
                    TextCard(phrases["divs.discovery.recommendation.message"]),
                    skillsRendererResult.divBody
                )
            ),
            state = null,
            analyticsInfo = AnalyticsInfo(
                intent = "personal_assistant.scenarios.skill_recommendation",
                objects = listOf(
                    SkillRecommendationsAnalyticsInfoObject(
                        recommendations.items.map { skillRecommendation -> skillRecommendation.skill.id },
                        RecommendationType.SKILLS_DISCOVERY_GC
                    )
                )
            ),
            isExpectsRequest = false,
            actions = skillsRendererResult.actions
        )
    }

    fun filterCandidates(
        candidates: List<SkillDiscoveryCandidate>,
        request: MegaMindRequest<Any>,
    ): List<SkillRecommendation> {
        logger.info("Received skill-id candidates {}", candidates)
        return skillRecommendationMapper.mapToSkillRecommendationList(
            candidates.sortedByDescending { it.relevance }
                .asSequence()
                .mapNotNull { candidate -> skillProvider.getSkill(candidate.skillId).orElse(null) }
                .filter { skillInfo ->
                    SkillFilters.VALID_FOR_RECOMMENDATIONS.test(skillInfo) &&
                        SkillFilters.EXPLICIT_CONTENT_RECOMMENDATION_FILTER.test(
                            request,
                            skillInfo
                        )
                }.take(MAX_RECOMMENDATIONS_SIZE),
            request
        ).also {
            logger.info("Filtered candidates ids {}", it.joinToString(",") { rec -> rec.skill.id })
        }
    }
}
