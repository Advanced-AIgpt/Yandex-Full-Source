package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.directives

import ru.yandex.alice.kronstadt.core.directive.CallbackDirective
import ru.yandex.alice.kronstadt.core.directive.Directive
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.onboarding.SkillsOnboardingDefinition

@Directive("skills_onboarding_scroll_next")
data class SkillsOnboardingScrollNextDirective(
    val type: SkillsOnboardingDefinition.SkillsOnboardingType
) : CallbackDirective
