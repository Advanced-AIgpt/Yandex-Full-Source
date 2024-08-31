package ru.yandex.alice.kronstadt.scenarios.discovery.implicit

import org.springframework.beans.factory.annotation.Value
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RelevantResponse
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.layout.Layout
import ru.yandex.alice.kronstadt.core.layout.TextCard
import ru.yandex.alice.kronstadt.core.scenario.AbstractScene
import ru.yandex.alice.kronstadt.core.text.Phrases
import ru.yandex.alice.kronstadt.scenarios.discovery.implicit.ImplicitSkillDiscoveryStationScene.Companion.findFirstActivationSemanticFrame
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType
import ru.yandex.alice.paskill.dialogovo.domain.RecommendationType
import ru.yandex.alice.paskill.dialogovo.domain.SkillRecommendation
import ru.yandex.alice.paskill.dialogovo.scenarios.RecommendationCardsRenderer
import ru.yandex.alice.paskill.dialogovo.scenarios.analytics.SkillRecommendationsAnalyticsInfoObject

@Component
open class GcWithDivCardsImplicitSkillsDiscoveryScene(
    private val recommendationCardsRenderer: RecommendationCardsRenderer,
    private val phrases: Phrases,
    @Value("\${storeUrl}")
    private val storeUrl: String
) : AbstractScene<Any, GcWithDivCardsImplicitSkillsDiscoveryScene.Args>(
    "IMPLICIT_SKILLS_DISCOVERY_GC_WITH_DIV_CARDS",
    argsClass = Args::class
) {

    class Args(val items: List<SkillRecommendation>)

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
                .recommendationType(RecommendationType.SKILLS_IMPLICIT_DISCOVERY)
                .recommendationSubType("implicit_discovery_megamind")
                .activationTypedSemanticFrame(findFirstActivationSemanticFrame(request.input.semanticFrames))
                .build()
        )
        val phrase = phrases["divs.discovery.implicit.recommendation.message"]
        return RunOnlyResponse(
            layout = Layout(
                outputSpeech = phrase,
                shouldListen = true,
                cards = listOf(
                    TextCard(phrase),
                    skillsRendererResult.divBody
                )
            ),
            state = null,
            analyticsInfo = AnalyticsInfo(
                intent = "personal_assistant.scenarios.skill_recommendation",
                objects = listOf(
                    SkillRecommendationsAnalyticsInfoObject(
                        recommendations.items.map { skillRecommendation -> skillRecommendation.skill.id },
                        RecommendationType.SKILLS_IMPLICIT_DISCOVERY
                    )
                )
            ),
            isExpectsRequest = false,
            actions = skillsRendererResult.actions
        )
    }
}
