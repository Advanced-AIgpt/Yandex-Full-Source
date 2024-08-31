package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.nlg

import ru.yandex.alice.kronstadt.core.layout.TextWithTts
import java.math.BigDecimal

interface Countable {
    fun getPluralForm(number: BigDecimal): TextWithTts
    fun pluralize(number: BigDecimal): TextWithTts

    fun pluralize(number: Long): TextWithTts = pluralize(BigDecimal(number))

    fun getPluralForm(number: Long): TextWithTts = getPluralForm(BigDecimal(number))
}
