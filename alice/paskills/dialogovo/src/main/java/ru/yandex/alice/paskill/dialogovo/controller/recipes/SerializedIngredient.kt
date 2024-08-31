package ru.yandex.alice.paskill.dialogovo.controller.recipes

import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.IngredientWithQuantity

internal data class SerializedIngredient(val components: List<Component>) {

    companion object {
        internal fun fromIngredient(ingredient: IngredientWithQuantity) = SerializedIngredient(
            listOf(Component(ingredient.toTextWithTtsWithIngredientName().text, false))
        )
    }

    internal data class Component(val text: String, val bold: Boolean)
}
