package ru.yandex.alice.paskill.dialogovo.controller.recipes

import ru.yandex.alice.kronstadt.core.layout.TextWithTts
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.nlg.CountableObject
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.nlg.TtsTag

internal data class NumberOfServings(val quantity: Int) {
    val text: String = SERVING.getPluralForm(quantity.toLong()).text

    companion object {
        private val SERVING = CountableObject(
            TextWithTts("порция"),
            TextWithTts("порции"),
            TextWithTts("порций"),
            TextWithTts("порции"),
            TtsTag.FEMININE
        )
    }
}
