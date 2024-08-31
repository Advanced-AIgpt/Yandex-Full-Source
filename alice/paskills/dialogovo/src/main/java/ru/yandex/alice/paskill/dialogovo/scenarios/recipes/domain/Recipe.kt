package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain

import com.fasterxml.jackson.annotation.JsonCreator
import com.fasterxml.jackson.annotation.JsonIgnore
import ru.yandex.alice.kronstadt.core.layout.TextWithTts
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.entities.KitchenEquipment
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.entities.KitchenEquipment.JsonRef.Companion.toKitchenEquipmentList
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.entities.NamedEntity
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.nlg.DurationToString
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.providers.IngredientProvider
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.providers.JsonEntityProvider.EntityNotFound
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.providers.KitchenEquipmentProvider
import java.time.Duration
import java.util.Optional

data class Recipe(
    override val id: String,
    val author: Optional<Author>,
    val isEnabled: Boolean,
    val alternativeIds: List<String>,
    val imageUrl: Optional<String>,
    override val name: TextWithTts,
    val inflectedNameCases: InflectedNameCases,
    val cookingTime: Duration,
    val numberOfServings: Int,
    override val ingredients: List<IngredientWithQuantity>,
    val equipmentList: List<KitchenEquipment>,
    @JsonIgnore val steps: List<RecipeStep>,
    val tags: List<RecipeTag>,
    val recommended: Boolean,
    val epigraph: Optional<TextWithTts>,
) : NamedEntity, WithIngredients {

    @get:JsonIgnore
    val cookingTimeText: TextWithTts = DurationToString.render(cookingTime)

    override fun toString(): String {
        return "Recipe(id=$id)"
    }

    fun nameWithAuthorGen(): TextWithTts {
        return if (author.isPresent) {
            inflectedNameCases.genitive
                .plus(FROM)
                .plus(author.get().gen)
        } else {
            inflectedNameCases.genitive
        }
    }

    /**
     * Returns true if stepId is the last step of the recipe or last step depends only timer created at stepId
     */
    fun isEffectivelyLastStep(stepId: Int): Boolean {
        val lastStepIndex = steps.size - 1
        val lastStep = steps[lastStepIndex]
        val lastStepDependsOnTimerFromStepId = lastStep.dependencies
            .filterIsInstance<TimerDependency>()
            .all { d: TimerDependency -> d.stepId == stepId }
        return stepId == lastStepIndex || stepId == lastStepIndex - 1 && lastStepDependsOnTimerFromStepId
    }

    data class InflectedNameCases(
        val genitive: TextWithTts,
        val accusative: TextWithTts
    ) {

        companion object {
            @JsonCreator
            private fun fromJson(
                genitive: TextWithTts?,
                accusative: TextWithTts?
            ): InflectedNameCases = InflectedNameCases(
                genitive ?: throw RuntimeException("Inflected name should contain genitive case"),
                accusative ?: throw RuntimeException("Inflected name should contain accusative case")
            )
        }
    }

    fun print(): String {
        val buf = StringBuilder()
        buf.append(name.text)
        buf.append("\n")
        buf.append("  Ингредиенты:\n")
        for (ingredient in ingredients) {
            buf.append("    ")
                .append(ingredient.toTextWithTtsWithIngredientName().text)
                .append("\n")
        }
        buf.append("  Шаги рецепта:\n")
        var i = 1
        for (step in steps) {
            buf.append("    ")
                .append(i++)
                .append(". ")
                .append(if (!step.dependencies.isEmpty()) "[Ждем шаг " +
                    step.dependencies.joinToString(separator = ", ") { (it.stepId + 1).toString() }
                    + "] " else "")
                .append(step.text)
                .append(if (step.hasTimer()) " [" + step.getTimer().duration.toMinutes() + " минут]" else "")
                .append("\n")
        }
        return buf.append("\n").toString()
    }

    val publicTags: List<RecipeTag>
        get() = tags.filter { tag: RecipeTag -> tag.type != RecipeTag.Type.HIDDEN }

    class Json(
        private val id: String?,
        private val author: Author?,
        private val enabled: Boolean?,
        private val alternativeIds: List<String>?,
        private val name: TextWithTts?,
        private val imageUrl: String?,
        private val inflectedNameCases: InflectedNameCases?,
        private val cookingTime: Duration?,
        private val numberOfServings: Int?,
        private val ingredients: List<IngredientWithQuantity.JsonRef>?,
        private val equipment: List<KitchenEquipment.JsonRef>?,
        private val steps: List<RecipeStep.Json>?,
        private val tags: List<String>?,
        private val recommended: Boolean?,
        private val epigraph: TextWithTts?
    ) {

        @Throws(EntityNotFound::class)
        fun toRecipe(
            ingredientProvider: IngredientProvider,
            kitchenEquipmentProvider: KitchenEquipmentProvider
        ): Recipe {
            val realIngredients = IngredientWithQuantity.JsonRef.toIngredientWithQuantityList(
                ingredients ?: listOf(),
                ingredientProvider
            )
            val realEquipment = toKitchenEquipmentList(equipment ?: listOf(), kitchenEquipmentProvider)
            val realSteps: List<RecipeStep>
            if (steps != null) {
                realSteps = ArrayList(steps.size)
                for (jsonStep in steps) {
                    realSteps.add(jsonStep.toRecipeStep(ingredientProvider))
                }
            } else {
                realSteps = listOf()
            }
            val realTags = (tags ?: listOf())
                .map { tag -> RecipeTag.STRING_ENUM_RESOLVER.fromValueOrThrow(tag) }
                .sortedWith(RecipeTag.COMPARATOR)
            return Recipe(
                id = id ?: throw RuntimeException("Recipe id cannot be null"),
                author = Optional.ofNullable(
                    author
                ),
                isEnabled = enabled ?: true,
                alternativeIds = alternativeIds ?: listOf(),
                imageUrl = Optional.ofNullable(imageUrl),
                name = name ?: throw RuntimeException("Recipe name cannot be null"),
                inflectedNameCases = inflectedNameCases
                    ?: throw RuntimeException("Recipe's inflected name cannot be null"),
                cookingTime = cookingTime ?: throw RuntimeException("Cooking time cannot be null"),
                numberOfServings = numberOfServings ?: throw RuntimeException("Number of servings cannot be null"),
                ingredients = realIngredients,
                equipmentList = realEquipment,
                steps = realSteps,
                tags = realTags,
                recommended = recommended ?: true,
                epigraph = Optional.ofNullable(epigraph)
            )
        }
    }

    data class Author(
        // имя в родительном падеже
        val gen: TextWithTts
    ) {

        companion object {
            @JsonCreator
            fun fromJson(gen: TextWithTts?): Author {
                return Author(gen ?: throw RuntimeException("Author's name cannot be null"))
            }
        }
    }

    companion object {
        private val FROM = TextWithTts("от")
    }
}
