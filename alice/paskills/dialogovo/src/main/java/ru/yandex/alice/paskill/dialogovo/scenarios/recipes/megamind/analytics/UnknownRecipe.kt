package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.analytics

import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoObject

class UnknownRecipe private constructor(name: String, humanReadable: String) :
    AnalyticsInfoObject("unknown_recipe", name, humanReadable) {
    companion object {
        @JvmStatic
        fun fromRecipeId(id: String): UnknownRecipe {
            return UnknownRecipe(id, "")
        }

        @JvmStatic
        fun fromWildcard(recipeWildcard: String): UnknownRecipe {
            return UnknownRecipe("wildcard", recipeWildcard)
        }
    }
}
