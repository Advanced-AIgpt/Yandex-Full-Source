package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.onboarding

import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo
import ru.yandex.alice.paskill.dialogovo.providers.recommendation.SkillRecommendationsProvider
import java.util.Random

class GamesOnboardingStationRecommendationsExperimentSwitcher(
    private val baseSkillsProvider: SkillRecommendationsProvider,
    private val experimentProvider: SkillRecommendationsProvider,
    private val experiment: String
) : SkillRecommendationsProvider {

    override fun getSkills(random: Random, n: Int, request: MegaMindRequest<*>): MutableList<SkillInfo> {
        if (request.hasExperiment(experiment)) {
            return experimentProvider.getSkills(random, n, request)
        }
        return baseSkillsProvider.getSkills(random, n, request)
    }
}
