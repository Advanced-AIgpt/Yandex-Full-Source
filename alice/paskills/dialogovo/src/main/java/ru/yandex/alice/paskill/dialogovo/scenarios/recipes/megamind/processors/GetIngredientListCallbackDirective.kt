package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.processors

import com.fasterxml.jackson.annotation.JsonProperty
import ru.yandex.alice.kronstadt.core.directive.CallbackDirective
import ru.yandex.alice.kronstadt.core.directive.Directive

@Directive("recipes_get_ingredient_list")
data class GetIngredientListCallbackDirective(
    @JsonProperty("recipe_id")
    val recipeId: String
) : CallbackDirective
