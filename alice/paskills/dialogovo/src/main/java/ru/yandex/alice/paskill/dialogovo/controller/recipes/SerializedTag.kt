package ru.yandex.alice.paskill.dialogovo.controller.recipes

import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.RecipeTag

class SerializedTag(tag: RecipeTag) {
    val value: String = tag.value()
}
