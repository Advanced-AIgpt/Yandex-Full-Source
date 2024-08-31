package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents

import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticFrames

data class SelectRecipeIntent(val recipeId: String) : RecipeIntent(NAME) {
    companion object {
        const val NAME = SemanticFrames.RECIPE_SELECT_RECIPE
    }
}
