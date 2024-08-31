package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents

import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.RecipeTag
import java.util.Optional

data class RecipeOnboardingIntent(val tag: Optional<RecipeTag>) : RecipeIntent(NAME) {
    companion object {
        const val NAME = "alice.recipes.onboarding"
    }
}
