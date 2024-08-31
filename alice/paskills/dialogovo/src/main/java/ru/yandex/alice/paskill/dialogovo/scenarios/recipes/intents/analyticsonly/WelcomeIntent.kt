package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.analyticsonly

import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.RecipeIntent

internal object WelcomeIntent : RecipeIntent("alice.recipes.welcome_message") {
    @JvmField
    val NAME = name
}
