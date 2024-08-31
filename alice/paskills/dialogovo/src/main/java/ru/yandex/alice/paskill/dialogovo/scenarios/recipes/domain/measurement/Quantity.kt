package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.measurement

import com.fasterxml.jackson.annotation.JsonSubTypes
import com.fasterxml.jackson.annotation.JsonTypeInfo
import ru.yandex.alice.kronstadt.core.layout.TextWithTts
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.measurement.ExactQuantity

@JsonTypeInfo(
    use = JsonTypeInfo.Id.NAME,
    include = JsonTypeInfo.As.PROPERTY,
    property = "type",
    defaultImpl = ExactQuantity::class
)
@JsonSubTypes(
    JsonSubTypes.Type(value = ExactQuantity::class, name = "exact"),
    JsonSubTypes.Type(value = ToTaste::class, name = "to_taste")
)
sealed interface Quantity {
    fun toTextWithTts(): TextWithTts
}
