package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.analytics

import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoObject
import ru.yandex.alice.megamind.protos.analytics.scenarios.dialogovo.Dialogovo
import ru.yandex.alice.megamind.protos.analytics.scenarios.dialogovo.Dialogovo.TEquipment
import ru.yandex.alice.megamind.protos.analytics.scenarios.dialogovo.Dialogovo.TIngredient
import ru.yandex.alice.megamind.protos.scenarios.AnalyticsInfo.TAnalyticsInfo.TObject
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.Recipe
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.entities.KitchenEquipment
import javax.annotation.Nonnull

class AnalyticsInfoRecipe(val recipe: Recipe) :
    AnalyticsInfoObject("recipe", recipe.id, String.format("Рецепт «%s»", recipe.name.text)) {
    @Nonnull
    public override fun fillProtoField(@Nonnull protoBuilder: TObject.Builder): TObject.Builder {
        val ingredients = recipe.ingredients.map { ingredientWithQuantity ->
            TIngredient.newBuilder()
                .setId(ingredientWithQuantity.ingredient.id)
                .setName(ingredientWithQuantity.ingredient.name.text)
                .setHumanReadableQuantity(ingredientWithQuantity.toTextWithTtsWithoutIngredientName().text)
                .build()
        }
        val equipments = recipe.equipmentList
            .map { (id, name): KitchenEquipment ->
                TEquipment.newBuilder()
                    .setId(id)
                    .setName(name.text)
                    .build()
            }
        return protoBuilder.setRecipe(
            Dialogovo.TRecipe.newBuilder()
                .addAllIngredients(ingredients)
                .addAllEquipment(equipments)
                .setNumberOfSteps(recipe.steps.size)
                .setNumberOfServings(recipe.numberOfServings)
                .setHumanReadableCookingTime(recipe.cookingTimeText.text)
                .build()
        )
    }

    override fun toString(): String {
        return "AnalyticsInfoRecipe(recipe=$recipe)"
    }
}
