package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents

import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticFrames

object PreviousStepIntent : RecipeIntent(SemanticFrames.RECIPE_PREVIOUS_STEP) {
    @JvmField
    val NAME = name
}
