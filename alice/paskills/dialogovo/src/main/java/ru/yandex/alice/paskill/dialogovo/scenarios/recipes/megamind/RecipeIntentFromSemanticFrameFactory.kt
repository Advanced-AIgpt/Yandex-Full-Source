package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind

import org.apache.logging.log4j.LogManager
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrame
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticSlotType
import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticSlotEntityType
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.RecipeTag
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.DeleteStateIntent
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.HowMuchTimeWillItTakeIntent
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.HowMuchToPutIntent
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.NextStepIntent
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.PreviousStepIntent
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.RecipeIntent
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.RecipeOnboardingIntent
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.RepeatIntent
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.SelectRecipeIntent
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.StopCookingIntent
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.TellEquipmentListIntent
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.TellIngredientListIntent
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.TimerAlarm
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.TimerHowMuchTimeLeft
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.TimerStopPlaying
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.providers.IngredientProvider
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.providers.JsonEntityProvider.EntityNotFound
import java.util.Optional

@Component
class RecipeIntentFromSemanticFrameFactory(private val ingredientProvider: IngredientProvider) {

    fun convert(frames: List<SemanticFrame>): List<RecipeIntent> {
        return frames.mapNotNull { frame: SemanticFrame -> this.convert(frame) }
    }

    private fun convert(frame: SemanticFrame): RecipeIntent? {
        val intentName = frame.name
        try {
            return when (intentName) {
                SelectRecipeIntent.NAME -> makeSelectRecipeIntent(frame)
                DeleteStateIntent.NAME -> DeleteStateIntent
                NextStepIntent.NAME -> NextStepIntent
                PreviousStepIntent.NAME -> PreviousStepIntent
                HowMuchToPutIntent.NAME -> makeHowMuchToPutIntent(frame, false)
                HowMuchToPutIntent.ELLIPSIS_NAME -> makeHowMuchToPutIntent(frame, true)
                TellIngredientListIntent.NAME -> TellIngredientListIntent
                RepeatIntent.NAME -> RepeatIntent
                TimerHowMuchTimeLeft.NAME -> TimerHowMuchTimeLeft
                TimerAlarm.NAME -> TimerAlarm
                TimerStopPlaying.NAME -> TimerStopPlaying
                HowMuchTimeWillItTakeIntent.NAME -> HowMuchTimeWillItTakeIntent
                TellEquipmentListIntent.NAME -> TellEquipmentListIntent
                StopCookingIntent.NAME -> StopCookingIntent
                RecipeOnboardingIntent.NAME -> makeRecipeOnboardingIntent(frame)
                else -> {
                    logger.error("Unknown intent: ${intentName}")
                    null
                }
            }
        } catch (e: InvalidSlotValueException) {
            logger.error("Failed to create intent from {}", frame, e)
        }
        return null
    }

    private fun makeSelectRecipeIntent(src: SemanticFrame): SelectRecipeIntent? {
        val recipeIntent = src.getTypedEntityValue(SemanticSlotType.RECIPE.value, SemanticSlotEntityType.RECIPE)
            ?.let { SelectRecipeIntent(it) }
        return recipeIntent
    }

    private fun makeHowMuchToPutIntent(src: SemanticFrame, isEllipsis: Boolean): HowMuchToPutIntent? {
        val ingredientId = src.getTypedEntityValue(
            SemanticSlotType.INGREDIENT.value,
            SemanticSlotEntityType.INGREDIENT
        )
        return if (ingredientId != null) {
            try {
                val ingredient = ingredientProvider[ingredientId]
                HowMuchToPutIntent(ingredient, isEllipsis)
            } catch (entityNotFound: EntityNotFound) {
                logger.error("Failed to find ingredient: {}", ingredientId)
                null
            }
        } else {
            null
        }
    }

    private fun makeRecipeOnboardingIntent(src: SemanticFrame): RecipeOnboardingIntent {
        val tag = src.getTypedEntityValue(SemanticSlotType.RECIPE_TAG.value, SemanticSlotEntityType.RECIPE_TAG)
            ?.let { RecipeTag.STRING_ENUM_RESOLVER.fromValueOrNull(it) }
        return RecipeOnboardingIntent(Optional.ofNullable(tag))
    }

    class InvalidSlotValueException(message: String) : RuntimeException(message)

    companion object {
        private val logger = LogManager.getLogger()
    }
}
