package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.entities

import com.fasterxml.jackson.annotation.JsonCreator
import ru.yandex.alice.kronstadt.core.layout.TextWithTts
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.nlg.TtsTag
import java.math.BigDecimal
import javax.annotation.Nonnull

data class CountableIngredient(
    override val id: String,
    override val name: TextWithTts,
    override val inflectedName: TextWithTts,
    override val pluralForms: List<TextWithTts>,
    override val ttsTag: TtsTag,
    override val children: List<String>
) : GenericCountableNamedEntity, Ingredient {

    @Nonnull
    override fun getPluralForm(number: BigDecimal): TextWithTts {
        return countableObject.getPluralForm(number)
    }

    companion object {
        @JsonCreator
        @JvmStatic
        fun fromJson(
            id: String?,
            name: String?,
            nameTts: String?,
            inflectedName: TextWithTts?,
            pluralForms: List<TextWithTts>?,
            ttsTag: TtsTag?,
            children: List<String>?
        ): CountableIngredient {
            return CountableIngredient(
                id ?: throw RuntimeException("Ingredient id cannot be null"),
                TextWithTts(name ?: throw RuntimeException("Ingredient name cannot be null"), nameTts),
                inflectedName ?: throw RuntimeException("CountableIngredient's inflectedName cannot be null"),
                pluralForms ?: throw RuntimeException("CountableIngredient's pluralForms cannot be null"),
                ttsTag ?: throw RuntimeException("CountableIngredient's ttsTag cannot be null"),
                children ?: listOf()
            )
        }
    }
}
