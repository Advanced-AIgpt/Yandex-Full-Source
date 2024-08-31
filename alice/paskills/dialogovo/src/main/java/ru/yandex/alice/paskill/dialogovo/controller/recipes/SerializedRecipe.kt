package ru.yandex.alice.paskill.dialogovo.controller.recipes

import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.Recipe

internal data class SerializedRecipe constructor(
    val id: String,
    val name: String,
    val inflectedName: String,
    val author: SerializedAuthor?,
    val imageUrl: String?,
    val cookingTime: SerializedDuration,
    val servings: NumberOfServings,
    val ingredients: List<SerializedIngredient>,
    val equipment: List<SerializedEquipment>,
    val tags: List<String>,
) {

    companion object {
        @JvmStatic
        fun createFromRecipe(recipe: Recipe): SerializedRecipe = SerializedRecipe(
            id = recipe.id,
            name = recipe.name.text,
            inflectedName = recipe.inflectedNameCases.accusative.text,
            author = recipe.author.map { author -> SerializedAuthor.fromAuthor(author) }.orElse(null),
            imageUrl = recipe.imageUrl.orElse(null),
            cookingTime = SerializedDuration.fromDuration(recipe.cookingTime),
            servings = NumberOfServings(recipe.numberOfServings),
            ingredients = recipe.ingredients
                .filter { ingredientWithQuantity -> !ingredientWithQuantity.isSilent }
                .map { ingredient -> SerializedIngredient.fromIngredient(ingredient) },
            equipment = recipe.equipmentList.map { equipment -> SerializedEquipment.fromEquipment(equipment) },
            tags = recipe.publicTags.map { obj -> obj.value() }
        )
    }
}

