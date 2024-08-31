package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.onboarding

import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.onboarding.response.SkillOnboardingStationResponseGenerator

class SkillsOnboardingDefinition(
    val semanticFrames: Set<String>,
    val type: SkillsOnboardingType,
    val responseGenerator: SkillOnboardingStationResponseGenerator,
    val experiment: String? = null
) {
    constructor(
        semanticFrame: String,
        type: SkillsOnboardingType,
        responseGenerator: SkillOnboardingStationResponseGenerator,
        experiment: String? = null
    ) : this(setOf(semanticFrame), type, responseGenerator, experiment)

    enum class SkillsOnboardingType {
        KIDS_GAMES_ONBOARDING,
        GAMES_ONBOARDING;
    }
}
