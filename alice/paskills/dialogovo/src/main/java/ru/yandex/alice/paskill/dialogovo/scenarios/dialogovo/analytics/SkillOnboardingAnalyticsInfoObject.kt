package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics

import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoObject
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.onboarding.SkillsOnboardingDefinition

data class SkillOnboardingAnalyticsInfoObject(
    val onboardingType: SkillsOnboardingDefinition.SkillsOnboardingType
) : AnalyticsInfoObject(onboardingType.name, "", "Онбординг навыков")
