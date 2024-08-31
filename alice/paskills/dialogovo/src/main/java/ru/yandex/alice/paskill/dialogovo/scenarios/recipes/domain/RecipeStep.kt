package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain

import ru.yandex.alice.kronstadt.core.layout.TextWithTts
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.providers.IngredientProvider
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.providers.JsonEntityProvider.EntityNotFound
import java.time.Duration
import java.util.Optional

data class RecipeStep(
    val text: String,
    private val tts: String?,
    override val ingredients: List<IngredientWithQuantity>,
    private val timer: Timer?,
    val dependencies: List<RecipeStepDependency>,
    val musicPostroll: Optional<MusicPostroll>,
) : WithIngredients {

    fun getTts(): String = tts ?: text

    fun toTextWithTts(): TextWithTts = TextWithTts(text, getTts())

    fun hasTimerDependencies(): Boolean = dependencies.any { it is TimerDependency }

    fun shouldSetTimer(): Boolean = timer != null

    fun getTimer(): Timer = timer ?: throw UnsupportedOperationException("This step doesn't support timer")

    fun hasTimer(): Boolean {
        return timer != null
    }

    data class Timer(
        val text: String,
        private val tts: String?,
        val duration: Duration,
    ) {

        fun toTextWithTts(): TextWithTts {
            return TextWithTts(text, tts)
        }

        fun getTts(): String {
            return tts ?: text
        }

        fun id(stepId: Int): String {
            return String.format("recipe_step_%d", stepId)
        }
    }

    class Json(
        private val text: String?,
        private val tts: String?,
        private val ingredients: List<IngredientWithQuantity.JsonRef>?,
        private val timer: Timer?,
        private val dependencies: List<RecipeStepDependency>?,
        private val musicPostroll: MusicPostroll?
    ) {

        @Throws(EntityNotFound::class)
        fun toRecipeStep(ingredientProvider: IngredientProvider): RecipeStep {
            val realIngredients = IngredientWithQuantity.JsonRef.toIngredientWithQuantityList(
                ingredients ?: listOf(),
                ingredientProvider
            )
            return RecipeStep(
                text ?: throw RuntimeException("Recipe step cannot be null"),
                tts,
                realIngredients,
                timer,
                dependencies ?: listOf(),
                Optional.ofNullable(musicPostroll)
            )
        }
    }
}
