package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain

import com.fasterxml.jackson.annotation.JsonCreator
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.RecipeState

data class TimerDependency(
    override val stepId: Int,
    val timerStepCompleted: StepCompletedDependency
) : RecipeStepDependency {

    @JsonCreator
    constructor(stepId: Int) : this(stepId, StepCompletedDependency(stepId))

    override fun isFulfilled(recipe: Recipe, state: RecipeState): Boolean {
        val timerId = recipe.steps[stepId].getTimer().id(stepId)
        val timer = state.timers.firstOrNull { it.id == timerId }
        return (timer?.isCurrentlyPlaying ?: true) &&
            timerStepCompleted.isFulfilled(recipe, state) &&
            state.createdTimerIds.contains(timerId)
    }
}
