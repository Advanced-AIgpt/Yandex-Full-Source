package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.analyticsonly

import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.RecipeIntent

/**
 * An intent that is used only in AnalyticsInfo and can't be created from webhook intent or semantic frame
 */
internal abstract class AnalyticsOnlyRecipeIntent(name: String) : RecipeIntent(name)
