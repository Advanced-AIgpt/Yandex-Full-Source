package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.processors

import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.paskill.dialogovo.megamind.processor.SingleSemanticFrameRunProcessor
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.RecipeState
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.RecipeState.TimerState
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.Recipe
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.providers.RecipeProvider
import java.time.Duration
import java.time.Instant
import java.util.Optional
import java.util.function.Predicate

abstract class InsideRecipeProcessor protected constructor(protected val recipeProvider: RecipeProvider) :
    SingleSemanticFrameRunProcessor<DialogovoState> {

    companion object {
        fun isInRecipe(recipeProvider: RecipeProvider, request: MegaMindRequest<DialogovoState>): Boolean {
            val recipe = request.state
                ?.recipesState
                ?.currentRecipeId
                ?.orElse(null)
                ?.let { recipeProvider[it].orElse(null) }

            return if (recipe == null) {
                false
            } else DidNotExceedTimeout(recipe).test(request)
        }
    }

    protected fun isInRecipe(request: MegaMindRequest<DialogovoState>): Boolean = isInRecipe(recipeProvider, request)

    override fun canProcess(request: MegaMindRequest<DialogovoState>): Boolean {
        return isInRecipe(request) && hasFrame().test(request)
    }

    protected fun getState(scenarioState: Optional<DialogovoState>): RecipeState {
        return scenarioState.get().getRecipesStateO().orElse(RecipeState.EMPTY)
    }

    protected fun getRecipe(state: Optional<DialogovoState>, provider: RecipeProvider): Recipe {
        val recipeId = state.get().recipesState?.currentRecipeId?.get()!!
        return provider[recipeId].orElseThrow { RuntimeException("Failed to find recipe with id $recipeId") }
    }

    private class DidNotExceedTimeout constructor(private val recipe: Recipe) :
        Predicate<MegaMindRequest<DialogovoState>>, WithNativeTimers {

        override fun test(request: MegaMindRequest<DialogovoState>): Boolean {
            val timers = request.state?.recipesState?.timers ?: listOf()
            val currentStep = request.state?.recipesState?.currentStepId?.orElse(null)
            val timeout: Duration = if (currentStep == null) {
                INTRO_TIMEOUT
            } else {
                timers.maxOfOrNull { t: TimerState -> t.duration.plus(FIVE_MINUTES) }
                    ?: (if (recipe.isEffectivelyLastStep(currentStep)) LAST_STEP_TIMEOUT else ORDINARY_STEP_TIMEOUT)
            }
            val prevRequestTimestamp =
                request.state?.prevResponseTimestamp?.let { Instant.ofEpochMilli(it) } ?: request.serverTime

            val timeSinceLastStep = Duration.between(prevRequestTimestamp, request.serverTime)
            return timeSinceLastStep <= timeout
        }

        companion object {
            private val FIVE_MINUTES = Duration.ofMinutes(5)
            private val ORDINARY_STEP_TIMEOUT = Duration.ofHours(3)
            private val INTRO_TIMEOUT = Duration.ofMinutes(3)
            private val LAST_STEP_TIMEOUT = Duration.ofMinutes(15)
        }
    }
}
