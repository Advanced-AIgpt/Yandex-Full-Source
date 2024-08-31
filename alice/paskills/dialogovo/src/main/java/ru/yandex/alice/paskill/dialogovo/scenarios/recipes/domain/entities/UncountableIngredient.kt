package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.entities

import com.fasterxml.jackson.annotation.JsonCreator
import ru.yandex.alice.kronstadt.core.layout.TextWithTts

data class UncountableIngredient internal constructor(
    override val id: String,
    override val name: TextWithTts,
    override val inflectedName: TextWithTts,
    override val children: List<String>
) : UncountableNamedEntity, Ingredient {

    companion object {

        @JvmStatic
        @JsonCreator
        fun fromJson(
            id: String?,
            name: String?,
            nameTts: String?,
            inflectedName: String?,
            inflectedNameTts: String?,
            children: List<String>?
        ): UncountableIngredient = UncountableIngredient(
            id ?: throw RuntimeException("Ingredient id cannot be null"),
            TextWithTts(name ?: throw RuntimeException("Ingredient text name cannot be null"), nameTts),
            TextWithTts(
                inflectedName ?: throw RuntimeException("Ingredient's inflected name cannot be null"),
                inflectedNameTts
            ),
            children ?: listOf()
        )
    }
}
