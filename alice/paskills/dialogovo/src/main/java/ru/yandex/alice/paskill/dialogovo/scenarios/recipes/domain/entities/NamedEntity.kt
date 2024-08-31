package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.entities

import com.fasterxml.jackson.annotation.JsonSubTypes
import com.fasterxml.jackson.annotation.JsonTypeInfo
import ru.yandex.alice.kronstadt.core.layout.TextWithTts
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.Recipe

@JsonTypeInfo(use = JsonTypeInfo.Id.NAME, include = JsonTypeInfo.As.PROPERTY, property = "type")
@JsonSubTypes(
    JsonSubTypes.Type(value = CountableIngredient::class, name = "countable_ingredient"),
    JsonSubTypes.Type(value = UncountableIngredient::class, name = "uncountable_ingredient"),
    JsonSubTypes.Type(value = KitchenEquipment::class, name = "equipment"),
    JsonSubTypes.Type(value = Recipe::class, name = "recipe")
)
interface NamedEntity {
    val id: String
    val name: TextWithTts
}
