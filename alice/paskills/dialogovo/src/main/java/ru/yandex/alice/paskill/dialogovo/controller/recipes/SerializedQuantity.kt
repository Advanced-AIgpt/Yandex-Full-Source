package ru.yandex.alice.paskill.dialogovo.controller.recipes

import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.measurement.ExactQuantity
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.measurement.Quantity
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.measurement.ToTaste
import java.math.BigDecimal

internal data class SerializedQuantity(
    var type: String,
    var value: BigDecimal?,
    var measurementUnit: String?,
    var text: String,
) {

    companion object {
        internal fun fromQuantity(quantity: Quantity): SerializedQuantity {
            return when (quantity) {
                is ToTaste -> {
                    SerializedQuantity(
                        type = "to_taste",
                        value = null,
                        measurementUnit = null,
                        text = ToTaste.toTextWithTts().text,
                    )
                }
                is ExactQuantity -> {
                    SerializedQuantity(
                        type = "exact",
                        value = quantity.value,
                        measurementUnit = quantity.measurementUnit.name,
                        text = "",
                    )
                }
                else -> {
                    throw UnsupportedOperationException("Unknown quantity type: " + quantity.javaClass.name)
                }
            }
        }
    }
}
