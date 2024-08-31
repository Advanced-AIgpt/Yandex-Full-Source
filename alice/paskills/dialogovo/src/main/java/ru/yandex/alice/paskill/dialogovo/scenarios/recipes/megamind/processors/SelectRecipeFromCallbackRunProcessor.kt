package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.processors

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.nlg.RecipeIntroReader
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.providers.RecipeProvider
import java.util.Optional

@Component
class SelectRecipeFromCallbackRunProcessor(
    recipeIntroReader: RecipeIntroReader,
    recipeProvider: RecipeProvider
) : SelectKnownRecipeRunProcessor(recipeIntroReader, recipeProvider) {

    override fun getRecipeId(request: MegaMindRequest<DialogovoState>): Optional<String> =
        SelectRecipeFromCallbackRunProcessor.getRecipeId(request)

    override val type: RunRequestProcessorType
        get() = RunRequestProcessorType.SELECT_RECIPE_CALLBACK

    companion object {
        fun getRecipeId(request: MegaMindRequest<DialogovoState>): Optional<String> {

            return if (request.input.isCallback(SelectRecipeDirective::class.java)) Optional.of(
                request.input.getDirective(SelectRecipeDirective::class.java).recipeId
            ) else Optional.empty()
        }
    }
}
