package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.analyticsonly

internal object RecipeFinishedIntent : AnalyticsOnlyRecipeIntent("alice.recipe.recipe_finished") {
    @JvmField
    val NAME = name
}
