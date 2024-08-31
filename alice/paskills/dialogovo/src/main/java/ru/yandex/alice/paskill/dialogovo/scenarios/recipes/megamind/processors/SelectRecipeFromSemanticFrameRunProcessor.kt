package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.processors

import org.apache.logging.log4j.LogManager
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticSlotType
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType
import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticFrames
import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticSlotEntityType
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.RecipeOnboardingIntent
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.nlg.RecipeIntroReader
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.providers.RecipeProvider
import java.util.Optional

@Component
class SelectRecipeFromSemanticFrameRunProcessor(
    recipeIntroReader: RecipeIntroReader,
    recipeProvider: RecipeProvider,
) : SelectKnownRecipeRunProcessor(recipeIntroReader, recipeProvider) {
    override fun getRecipeId(request: MegaMindRequest<DialogovoState>): Optional<String> {
        val canUseEllipsis = (request.state
            ?.recipesState
            ?.previousIntent
            ?.orElse(null)
            ?.let { RecipeOnboardingIntent.NAME == it }
            ?: false) && !request.newScenarioSession

        var semanticFrame = request.getSemanticFrame(SemanticFrames.RECIPE_SELECT_RECIPE)
        if (semanticFrame != null) {
            // log alice.recipes.select_recipe frame for analytics
            logger.info("Processing alice.recipes.select_recipe frame: {}", semanticFrame)
        }
        if (semanticFrame == null && canUseEllipsis) {
            semanticFrame = request.getSemanticFrame(SemanticFrames.RECIPE_SELECT_RECIPE_ELLIPSIS)
        }
        return Optional.ofNullable(
            semanticFrame?.getTypedEntityValue(
                SemanticSlotType.RECIPE.value,
                SemanticSlotEntityType.RECIPE
            )
        )
    }

    override val type: RunRequestProcessorType
        get() = RunRequestProcessorType.SELECT_RECIPE

    companion object {
        private val logger = LogManager.getLogger(SelectRecipeFromSemanticFrameRunProcessor::class.java)
    }
}
