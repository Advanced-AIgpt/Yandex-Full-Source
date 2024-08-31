package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.nlg

import org.springframework.beans.factory.annotation.Qualifier
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.layout.TextWithTts
import ru.yandex.alice.kronstadt.core.text.Phrases
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.Recipe
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.processors.ingredients.RequiredListRenderer
import java.util.Random

@Component
class RecipeIntroReader(
    private val requiredListRenderer: RequiredListRenderer,
    @param:Qualifier("recipePhrases") private val phrases: Phrases
) {
    fun readIntro(
        recipe: Recipe,
        random: Random,
        renderIngredientsAsList: Boolean
    ): TextWithTts {
        val ingredientList = requiredListRenderer.renderRequiredList(
            recipe, renderIngredientsAsList, random, false
        )

        val cookingTimePhrase = phrases.getRandomTextWithTts(
            "cooking_time", random, recipe.cookingTimeText
        )

        return if (recipe.epigraph.isPresent) {
            phrases.getRandomTextWithTts(
                "select_recipe_with_epigraph.success",
                random,
                recipe.epigraph.get(),
                cookingTimePhrase,
                ingredientList
            )
        } else {
            phrases.getRandomTextWithTts(
                "select_recipe.success",
                random,
                recipe.nameWithAuthorGen(),
                cookingTimePhrase,
                ingredientList
            )
        }
    }
}
