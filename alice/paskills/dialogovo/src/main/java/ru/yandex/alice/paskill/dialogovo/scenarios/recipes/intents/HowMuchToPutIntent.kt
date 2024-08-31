package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents

import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticFrames
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.entities.Ingredient

data class HowMuchToPutIntent(val ingredient: Ingredient, val isEllipsis: Boolean) : RecipeIntent(NAME) {
    companion object {
        const val NAME = SemanticFrames.RECIPE_HOW_MUCH_TO_PUT
        const val ELLIPSIS_NAME = SemanticFrames.RECIPE_HOW_MUCH_TO_PUT_ELLIPSIS
    }
}
