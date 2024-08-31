package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain

import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.RecipeState

data class StepCompletedDependency(override val stepId: Int) : RecipeStepDependency {
    override fun isFulfilled(recipe: Recipe, state: RecipeState): Boolean =
        state.completedSteps.contains(stepId) ||
            (state.currentStepId.isPresent && state.currentStepId.get() == stepId)
}
