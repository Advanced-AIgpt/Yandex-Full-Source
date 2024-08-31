package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.measurement

import ru.yandex.alice.kronstadt.core.layout.TextWithTts

object ToTaste : Quantity {
    private val TEXT = TextWithTts("по вкусу")
    const val SERIALIZED_VALUE = "to_taste"

    override fun toTextWithTts(): TextWithTts {
        return TEXT
    }
}
