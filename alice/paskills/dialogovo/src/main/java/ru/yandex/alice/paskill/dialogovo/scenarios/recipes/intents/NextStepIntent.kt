package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents

import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticFrames

internal object NextStepIntent : RecipeIntent(SemanticFrames.RECIPE_NEXT_STEP) {
    @JvmField
    val NAME = name
}
