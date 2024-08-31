package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.navigation

import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.RecipeStep

class RecipeStepWithPosition(val step: RecipeStep, val position: Int) : RecipePosition
