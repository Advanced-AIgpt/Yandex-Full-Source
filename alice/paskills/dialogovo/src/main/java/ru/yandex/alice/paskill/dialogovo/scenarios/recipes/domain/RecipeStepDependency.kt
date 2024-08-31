package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain

import com.fasterxml.jackson.annotation.JsonSubTypes
import com.fasterxml.jackson.annotation.JsonTypeInfo
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.RecipeState
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.TimerDependency

@JsonTypeInfo(use = JsonTypeInfo.Id.NAME, include = JsonTypeInfo.As.PROPERTY, property = "type")
@JsonSubTypes(
    JsonSubTypes.Type(value = StepCompletedDependency::class, name = "step_completed"),
    JsonSubTypes.Type(value = TimerDependency::class, name = "timer")
)
sealed interface RecipeStepDependency {
    fun isFulfilled(recipe: Recipe, state: RecipeState): Boolean
    val stepId: Int
}
