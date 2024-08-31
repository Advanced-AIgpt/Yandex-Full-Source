package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.processors.ingredients

import org.springframework.beans.factory.annotation.Qualifier
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.layout.TextWithTts
import ru.yandex.alice.kronstadt.core.text.ListRenderer
import ru.yandex.alice.kronstadt.core.text.Phrases
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.RecipeConfig
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.Recipe
import java.util.Optional
import java.util.Random

@Component
class RequiredListRenderer internal constructor(
    @param:Qualifier("recipePhrases") private val phrases: Phrases,
    private val recipeConfig: RecipeConfig
) {
    fun renderRequiredList(
        recipe: Recipe,
        renderIngredientsAsList: Boolean,
        random: Random,
        usePauses: Boolean
    ): TextWithTts {

        val requiredItems: List<TextWithTts> =
            recipe.equipmentList.map { it.toTextWithTts() } +
                recipe.ingredients
                    .filter { ingredient -> !ingredient.isSilent }
                    .map { ingredient -> ingredient.toTextWithTtsWithIngredientName() }

        val pause = if (usePauses) Optional.of(recipeConfig.slowIngredientListPauseMs) else Optional.empty()
        val textWithTts: TextWithTts = if (renderIngredientsAsList) {
            val list = ListRenderer.renderWithLineBreaks(requiredItems, pause)
            phrases.getRandomTextWithTts("you_will_need_line_break", random, list)
        } else {
            val list = ListRenderer.render(requiredItems, pause)
            val phrasesKey = if (pause.isPresent) "you_will_need_slow" else "you_will_need"
            phrases.getRandomTextWithTts(phrasesKey, random, list)
        }
        return textWithTts
    }
}
