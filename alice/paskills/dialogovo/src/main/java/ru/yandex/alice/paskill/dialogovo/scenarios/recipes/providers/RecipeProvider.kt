package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.providers

import org.apache.logging.log4j.LogManager
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.IngredientWithQuantity
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.Recipe
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.RecipeTag
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.entities.Ingredient
import java.util.Optional
import java.util.Random
import kotlin.math.min

typealias RecipeFilter = (Recipe) -> Boolean

class RecipeProvider(private val recipeMap: Map<String, Recipe>) {
    private val recipeByAlternativeId: Map<String, List<Recipe>> = recipeMap.values
        .flatMap { recipe -> recipe.alternativeIds.map { alternativeId -> alternativeId to recipe } }
        .groupBy({ (id, _) -> id }) { (_, recipe) -> recipe }

    operator fun get(id: String): Optional<Recipe> {
        return Optional.ofNullable(
            recipeMap[id]
                ?: (recipeByAlternativeId[id]?.firstOrNull())
        )
    }

    /**
     * Returns random recipe with isRecommended=true
     *
     * @param allowedIds Allow only specific skill ids for test stability. Ignored if empty.
     */
    fun singleRandomRecipe(random: Random, allowedIds: Set<String>, blacklist: Set<String>): Recipe {
        return random(random, allowedIds, blacklist, emptyList(), 1).first()
    }

    fun random(random: Random, allowedIds: Set<String>, blacklist: Set<String>, maxSize: Int): List<Recipe> {
        return random(random, allowedIds, blacklist, emptyList(), maxSize)
    }

    fun randomWithFilters(
        random: Random,
        tags: List<RecipeTag>,
        ingredients: List<Ingredient>,
        allowedIds: Set<String>,
        blacklist: Set<String>,
        maxCount: Int,
        ingredientProvider: IngredientProvider
    ): List<Recipe> {

        val filters: List<RecipeFilter> =
            tags.map { TagFilter(it) } + ingredients.map { IngredientFilter(it, ingredientProvider) }

        return random(random, allowedIds, blacklist, filters, maxCount)
    }

    fun randomWithTag(
        random: Random,
        tag: RecipeTag,
        allowedIds: Set<String>,
        blacklist: Set<String>,
        maxCount: Int
    ): List<Recipe> {
        return random(random, allowedIds, blacklist, listOf(TagFilter(tag)), maxCount)
    }

    fun randomWithIngredient(
        random: Random,
        ingredient: Ingredient,
        ingredientProvider: IngredientProvider,
        allowedIds: Set<String>,
        blacklist: Set<String>,
        maxCount: Int
    ): List<Recipe> {
        return random(
            random,
            allowedIds,
            blacklist,
            listOf(IngredientFilter(ingredient, ingredientProvider)),
            maxCount
        )
    }

    private fun random(
        random: Random,
        allowedIds: Set<String>,
        blacklist: Set<String>,
        filters: List<RecipeFilter>,
        maxSize: Int
    ): List<Recipe> {
        val totalFilters: List<RecipeFilter> = filters +
            Recipe::recommended +
            if (allowedIds.isNotEmpty()) listOf(IdFilter(allowedIds)) else listOf()

        val filteredRecipes = recipeMap.values.filter { recipe -> totalFilters.all { test -> test(recipe) } }

        // ignore blacklist if nothing has found
        val recipes = filteredRecipes.filter(BlacklistFilter(blacklist)).ifEmpty { filteredRecipes }

        if (recipes.isEmpty()) {
            logger.warn("Failed to find any recipes matching {}", filters)
        }
        return recipes.shuffled(random).take(maxSize)
    }

    fun size(): Int = recipeMap.size

    val allRecipes: Collection<Recipe>
        get() = recipeMap.values
    val allEnabledRecipes: List<Recipe>
        get() = recipeMap.values.filter { it.isEnabled && it.recommended }

    fun similar(recipe: Recipe, count: Int): List<Recipe> {
        val tags: Set<RecipeTag> = HashSet(recipe.tags)
        val recipesWithSimilarTags: MutableMap<Int, MutableList<Recipe>> = HashMap(tags.size + 1)
        for (tagCount in 0..tags.size) {
            recipesWithSimilarTags[tagCount] = ArrayList()
        }

        for (anotherRecipe in allEnabledRecipes) {
            if (anotherRecipe.id == recipe.id) {
                continue
            }
            val similarTagCount = anotherRecipe.tags.count { o: RecipeTag -> tags.contains(o) }
            recipesWithSimilarTags[similarTagCount]!!.add(anotherRecipe)
        }
        val similar: MutableList<Recipe> = ArrayList(count)
        for (similarTagCount in tags.size downTo 0) {
            if (similar.size >= count) break

            val recipes: List<Recipe> = recipesWithSimilarTags[similarTagCount]?.shuffled() ?: listOf()
            if (recipes.isNotEmpty()) {
                val itemsToPick = min(recipes.size, count - similar.size)
                similar.addAll(recipes.subList(0, itemsToPick))
            }
        }
        return similar.shuffled()
    }

    private data class TagFilter(private val tag: RecipeTag) : RecipeFilter {
        override fun invoke(recipe: Recipe): Boolean = recipe.tags.contains(tag)
    }

    private data class IngredientFilter(private val allowedIngredientIds: Set<Ingredient>) : RecipeFilter {
        constructor(ingredient: Ingredient, ingredientProvider: IngredientProvider) : this(
            ingredient.getAllChildren(ingredientProvider).toHashSet()
        )

        override fun invoke(recipe: Recipe): Boolean {
            return recipe.ingredients.stream()
                .map(IngredientWithQuantity::ingredient)
                .anyMatch { o: Ingredient -> allowedIngredientIds.contains(o) }
        }
    }

    private data class IdFilter(private val allowedIds: Set<String>) : RecipeFilter {
        override fun invoke(recipe: Recipe): Boolean = allowedIds.contains(recipe.id)
    }

    private data class BlacklistFilter(private val idFilter: IdFilter) : RecipeFilter {
        constructor(blacklist: Set<String>) : this(IdFilter(blacklist))

        override fun invoke(recipe: Recipe): Boolean = !idFilter.invoke(recipe)
    }

    companion object {
        private val logger = LogManager.getLogger(RecipeProvider::class.java)
    }
}
