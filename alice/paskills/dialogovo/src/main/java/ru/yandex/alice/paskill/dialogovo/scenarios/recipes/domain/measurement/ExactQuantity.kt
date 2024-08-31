package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.measurement

import ru.yandex.alice.kronstadt.core.layout.TextWithTts
import java.math.BigDecimal

data class ExactQuantity(val value: BigDecimal, val measurementUnit: MeasurementUnit) : Quantity {

    constructor(value: String, measurementUnit: MeasurementUnit) : this(BigDecimal(value), measurementUnit)

    override fun toTextWithTts(): TextWithTts {
        return measurementUnit.pluralize(value)
    }
}
