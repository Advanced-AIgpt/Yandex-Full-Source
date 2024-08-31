package ru.yandex.alice.paskill.dialogovo.controller.recipes

import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.Recipe

internal data class SerializedAuthor(val genitive: String) {

    companion object {
        @JvmStatic
        internal fun fromAuthor(author: Recipe.Author) = SerializedAuthor(author.gen.text)
    }
}
