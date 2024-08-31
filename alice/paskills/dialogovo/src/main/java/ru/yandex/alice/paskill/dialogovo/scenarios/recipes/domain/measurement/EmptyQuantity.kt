package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.measurement

import ru.yandex.alice.kronstadt.core.layout.TextWithTts

object EmptyQuantity : Quantity {
    override fun toTextWithTts(): TextWithTts = TextWithTts.EMPTY
}
