package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.onboarding

import org.springframework.beans.factory.annotation.Qualifier
import org.springframework.context.annotation.Bean
import org.springframework.context.annotation.Configuration
import ru.yandex.alice.kronstadt.core.text.Phrases
import ru.yandex.alice.paskill.dialogovo.providers.recommendation.SkillByScoreAndCategoryRecommendationProvider
import ru.yandex.alice.paskill.dialogovo.providers.recommendation.SkillRecommendationsProvider
import ru.yandex.alice.paskill.dialogovo.providers.recommendation.SkillTagsRecommendationProvider
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillCategoryKey
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillProvider
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillTagsKey
import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticFrames
import ru.yandex.alice.paskill.dialogovo.scenarios.SkillsSuggestVoiceButtonFactory
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.onboarding.response.SkillOnboardingResponseGeneratorExperimentSwitcher
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.onboarding.response.SuggestOneSkill
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.onboarding.response.SuggestOneSkillWithDescription
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.onboarding.response.SuggestOneSkillWithDescriptionAndNext
import ru.yandex.alice.paskill.dialogovo.service.SurfaceChecker

@Configuration
internal open class SkillsOnboardingConfiguration {

    @Bean("SkillsOnboardingStationRunProcessor")
    open fun skillsOnboardingStationRunProcessor(
        onboardings: List<SkillsOnboardingDefinition>
    ): SkillsOnboardingStationRunProcessor {
        return SkillsOnboardingStationRunProcessor(onboardings)
    }

    @Bean("gamesOnboardingStationRecommendationsProvider")
    open fun gamesOnboardingStationRecommendationsProvider(
        skillProvider: SkillProvider,
        surfaceChecker: SurfaceChecker
    ): SkillRecommendationsProvider =
        SkillByScoreAndCategoryRecommendationProvider(
            skillProvider,
            surfaceChecker,
            50,
            SkillCategoryKey.GAMES_TRIVIA_ACCESSORIES
        )

    @Bean("gamesOnboardingStationDefinition")
    open fun gamesOnboardingStationDefinition(
        voiceButtonFactory: SkillsSuggestVoiceButtonFactory,
        phrases: Phrases,
        skillProvider: SkillProvider,
        surfaceChecker: SurfaceChecker,
        @Qualifier("gamesOnboardingStationRecommendationsProvider") skillsProvider: SkillRecommendationsProvider,
    ): SkillsOnboardingDefinition = SkillsOnboardingDefinition(
        setOf(SemanticFrames.GAMES_ONBOARDING, SemanticFrames.GAME_SUGGEST),
        SkillsOnboardingDefinition.SkillsOnboardingType.GAMES_ONBOARDING,
        SkillOnboardingResponseGeneratorExperimentSwitcher(
            SuggestOneSkill(
                voiceButtonFactory,
                phrases,
                "station.games_onboarding.suggest",
                skillsProvider
            ),
            SuggestOneSkillWithDescription(
                voiceButtonFactory,
                phrases,
                "station.games_onboarding.suggest",
                "station.games_onboarding.suggest.with.description",
                skillsProvider
            ),
            SuggestOneSkillWithDescriptionAndNext(
                voiceButtonFactory,
                phrases,
                "station.games_onboarding.suggest.with.next",
                "station.games_onboarding.suggest.with.next.with.description",
                "station.games_onboarding.suggest.scroll.next",
                "station.games_onboarding.suggest.scroll.next.with.description",
                skillsProvider,
                SkillsOnboardingDefinition.SkillsOnboardingType.GAMES_ONBOARDING
            )
        )
    )

    @Bean("kidsGamesOnboardingDefinition")
    open fun kidsGamesOnboardingDefinition(
        voiceButtonFactory: SkillsSuggestVoiceButtonFactory,
        phrases: Phrases,
        skillProvider: SkillProvider,
        surfaceChecker: SurfaceChecker
    ): SkillsOnboardingDefinition = SkillsOnboardingDefinition(
        SemanticFrames.KIDS_GAMES_ONBOARDING,
        SkillsOnboardingDefinition.SkillsOnboardingType.KIDS_GAMES_ONBOARDING,
        SuggestOneSkill(
            voiceButtonFactory,
            phrases,
            "station.games_onboarding.suggest",
            SkillTagsRecommendationProvider(
                skillProvider,
                surfaceChecker,
                SkillTagsKey.KIDS_GAMES_ONBOARDING
            )
        )
    )
}
