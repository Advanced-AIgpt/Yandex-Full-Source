package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain

import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.entities.Ingredient
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.providers.IngredientProvider

interface WithIngredients {
    val ingredients: List<IngredientWithQuantity>
    fun findIngredient(
        requestedIngredient: Ingredient,
        ingredientProvider: IngredientProvider
    ): List<IngredientWithQuantity> {
        for (testIngredient in ingredients) {
            if (testIngredient.ingredient.id == requestedIngredient.id) {
                return listOf(testIngredient)
            }
        }
        // TODO: fill Ingredient's children instead of ids at app startup so that we won't need IngredientProvider here
        val children: Set<Ingredient> = requestedIngredient.getAllChildren(ingredientProvider)
        return ingredients.filter { ingredientWithQuantity -> children.contains(ingredientWithQuantity.ingredient) }
    }
}
