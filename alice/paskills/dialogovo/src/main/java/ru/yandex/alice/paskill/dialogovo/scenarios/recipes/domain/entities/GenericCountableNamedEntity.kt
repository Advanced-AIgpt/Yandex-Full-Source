package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.entities

import ru.yandex.alice.kronstadt.core.layout.TextWithTts
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.nlg.CountableObject
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.nlg.TtsTag
import java.math.BigDecimal

internal sealed interface GenericCountableNamedEntity : CountableNamedEntity {

    override val id: String
    override val name: TextWithTts
    val inflectedName: TextWithTts
    val pluralForms: List<TextWithTts>
    val ttsTag: TtsTag

    val countableObject: CountableObject
        get() = CountableObject.fromTextWithTtsList(pluralForms, ttsTag, inflectedName)

    override fun pluralize(number: BigDecimal): TextWithTts {
        return countableObject.pluralize(number)
    }
}
