package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain

import com.fasterxml.jackson.annotation.JsonCreator
import ru.yandex.alice.kronstadt.core.layout.TextWithTts
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.entities.CountableIngredient
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.entities.Ingredient
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.entities.UncountableIngredient
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.measurement.EmptyQuantity
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.measurement.ExactQuantity
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.measurement.MeasurementUnit
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.measurement.Quantity
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.measurement.ToTaste
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.providers.IngredientProvider
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.providers.JsonEntityProvider.EntityNotFound

class IngredientWithQuantity(
    val ingredient: Ingredient,
    val quantity: Quantity,
    val customText: CustomText?,
    val isSilent: Boolean
) {

    fun toTextWithTtsWithoutIngredientName(): TextWithTts {
        return if (ingredient is CountableIngredient && quantity is ExactQuantity) {
            ingredient.pluralize(quantity.value)
        } else {
            quantity.toTextWithTts()
        }
    }

    fun toTextWithTtsWithIngredientName(): TextWithTts {
        return if (ingredient is CountableIngredient) {
            // hacky way to add ttstag to quantity
            // TODO: refactor, use CountableIngredient.pluralize()
            val ttsTagValue = ingredient.ttsTag.tag()
            val ttsTag = TextWithTts("", ttsTagValue)
            val textQuantity = ttsTag.plus(quantity.toTextWithTts())
            if (customText != null) {
                if (customText.pronounceAmount) {
                    TextWithTts(
                        textQuantity.text + " " + customText.text,
                        textQuantity.tts + " " + customText.tts
                    )
                } else {
                    customText.delegate
                }
            } else if (quantity is ToTaste) {
                ingredient.name.plus(quantity.toTextWithTts(), " ")
            } else if (quantity is ExactQuantity) {
                ingredient.pluralize(quantity.value)
            } else {
                throw RuntimeException(String.format("Failed to generate ingredient text from %s", this))
            }
        } else if (ingredient is UncountableIngredient) {
            val textQuantity = quantity.toTextWithTts()
            val ingredientText = customText?.delegate ?: ingredient.inflectedName
            if (customText != null && quantity is ToTaste) {
                ingredientText
            } else if (quantity is ToTaste) {
                ingredientText.plus(textQuantity)
            } else {
                textQuantity.plus(ingredientText)
            }
        } else {
            throw RuntimeException(String.format("Unsupported Ingredient subclass %s", ingredient))
        }
    }

    data class JsonRef(
        private val id: String,
        private val amount: String,
        private val measurementUnit: String?,
        private val customText: CustomText?,
        private val silent: Boolean?
    ) {

        @Throws(EntityNotFound::class)
        fun toIngredientWithQuantity(
            ingredientProvider: IngredientProvider
        ): IngredientWithQuantity {
            val parsedQuantity = parseQuantity(amount, measurementUnit)
            return IngredientWithQuantity(
                ingredientProvider[id],
                parsedQuantity,
                customText,
                silent ?: false
            )
        }

        private fun parseQuantity(quantity: String?, measurementUnit: String?): Quantity = if (quantity == null) {
            EmptyQuantity
        } else if (ToTaste.SERIALIZED_VALUE == quantity) {
            ToTaste
        } else {
            ExactQuantity(quantity, MeasurementUnit.valueOf(measurementUnit!!))
        }

        companion object {
            @Throws(EntityNotFound::class)
            fun toIngredientWithQuantityList(
                jsonIngredients: List<JsonRef>,
                ingredientProvider: IngredientProvider
            ): List<IngredientWithQuantity> = jsonIngredients.map { it.toIngredientWithQuantity(ingredientProvider) }
        }
    }

    data class CustomText(
        val delegate: TextWithTts,
        val pronounceAmount: Boolean
    ) {

        @JsonCreator
        internal constructor(
            text: String,
            tts: String?,
            pronounceAmount: Boolean?
        ) : this(delegate = TextWithTts(text, tts), pronounceAmount = pronounceAmount ?: true)

        val text: String
            get() = delegate.text
        val tts: String
            get() = delegate.tts
    }
}
