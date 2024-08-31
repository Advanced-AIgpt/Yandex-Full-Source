package ru.yandex.alice.paskill.dialogovo.controller.recipes

import org.springframework.http.HttpStatus
import org.springframework.web.bind.annotation.GetMapping
import org.springframework.web.bind.annotation.PathVariable
import org.springframework.web.bind.annotation.RequestMapping
import org.springframework.web.bind.annotation.RequestParam
import org.springframework.web.bind.annotation.RestController
import org.springframework.web.server.ResponseStatusException
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.Recipe
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.RecipeTag
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.providers.RecipeProvider
import java.util.function.Supplier

@RestController
@RequestMapping("/store/recipes")
internal class RecipeStoreController(private val recipeProvider: RecipeProvider) {
    @GetMapping("/all")
    fun all(@RequestParam tag: String?): Collection<SerializedRecipe> {
        return recipeProvider.allEnabledRecipes
            .filter { recipe: Recipe ->
                tag == null || recipe.tags.any { obj: RecipeTag -> tag.equals(obj.value(), ignoreCase = true) }
            }
            .map(SerializedRecipe::createFromRecipe)
    }

    @GetMapping("/{id}")
    fun one(@PathVariable id: String): SerializedRecipe {
        return recipeProvider[id]
            .map(SerializedRecipe::createFromRecipe)
            .orElseThrow(NOT_FOUND)
    }

    @GetMapping("/{id}/similar")
    fun similar(
        @PathVariable id: String,
        @RequestParam(defaultValue = "3") count: Int
    ): List<SerializedRecipe> {
        return recipeProvider[id]
            .map { r: Recipe -> recipeProvider.similar(r, count).map { SerializedRecipe.createFromRecipe(it) } }
            .orElseThrow(NOT_FOUND)
    }

    @GetMapping("/tags")
    fun tags(): List<SerializedTag> {
        val unsortedTags: Map<RecipeTag, Int> = recipeProvider.allEnabledRecipes
            .flatMap { it.publicTags }
            .groupingBy { it }
            .eachCount()
        return unsortedTags
            .filterValues { it > 1 }
            .keys
            .sortedWith(RecipeTag.COMPARATOR)
            .map { SerializedTag(it) }
    }

    companion object {
        private val NOT_FOUND = Supplier { ResponseStatusException(HttpStatus.NOT_FOUND, "Recipe not found") }
    }
}
