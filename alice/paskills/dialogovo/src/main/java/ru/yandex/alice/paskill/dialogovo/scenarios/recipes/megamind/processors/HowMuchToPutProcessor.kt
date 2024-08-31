package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.processors

import org.apache.logging.log4j.util.Strings
import org.springframework.beans.factory.annotation.Qualifier
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.BaseRunResponse
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.layout.Layout.Companion.textWithOutputSpeech
import ru.yandex.alice.kronstadt.core.layout.TextWithTts
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticSlotType
import ru.yandex.alice.kronstadt.core.text.ListRenderer.render
import ru.yandex.alice.kronstadt.core.text.Phrases
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState.Companion.createRecipeState
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType
import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticFrames
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.IngredientWithQuantity
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.entities.Ingredient
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.HowMuchToPutIntent
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.analytics.AnalyticsInfoRecipe
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.analytics.AnalyticsInfoRecipeSession
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.providers.IngredientProvider
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.providers.JsonEntityProvider.EntityNotFound
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.providers.RecipeProvider
import java.util.Random

@Component
class HowMuchToPutProcessor(
    recipeProvider: RecipeProvider,
    private val ingredientProvider: IngredientProvider,
    @param:Qualifier("recipePhrases") private val phrases: Phrases,
    private val irrelevantResponseFactory: WeAreNotCookingResponse.Factory
) : InsideRecipeProcessor(recipeProvider) {
    override fun getSemanticFrame(): String {
        return SemanticFrames.RECIPE_HOW_MUCH_TO_PUT
    }

    override val type: RunRequestProcessorType
        get() = RunRequestProcessorType.HOW_MUCH_TO_PUT

    override fun canProcess(request: MegaMindRequest<DialogovoState>): Boolean {
        return (Predicates.SURFACE_IS_SUPPORTED.test(request)
            && isInRecipe(request)
            && request.hasAnySemanticFrame(
            SemanticFrames.RECIPE_HOW_MUCH_TO_PUT, SemanticFrames.RECIPE_HOW_MUCH_TO_PUT_ELLIPSIS
        ))
    }

    override fun process(context: Context, request: MegaMindRequest<DialogovoState>): BaseRunResponse<DialogovoState> {
        val recipe = getRecipe(request.getStateO(), recipeProvider)
        val stepId = request.state?.recipesState?.currentStepId?.orElse(null) ?: 0

        val slots = parseSlots(request) ?: return irrelevantResponseFactory.create(request)

        val requestedIngredient: Ingredient = slots.ingredient

        val ingredients = recipe.steps[stepId].findIngredient(requestedIngredient, ingredientProvider)
            .ifEmpty { recipe.findIngredient(requestedIngredient, ingredientProvider) }

        val textWithTts: TextWithTts = if (ingredients.size == 1) {
            // render short response only with quantity
            if (slots.showIngredientName) {
                ingredients[0].toTextWithTtsWithIngredientName()
            } else {
                ingredients[0].toTextWithTtsWithoutIngredientName()
            }
        } else {
            // render ingredient list with quantities and ingredient names
            renderIngredientList(request.random, ingredients)
        }

        val layout = textWithOutputSpeech(textWithTts, request.isVoiceSession())
        val state = request.state?.recipesState!!

        val newState = state.withPreviousIntent(HowMuchToPutIntent.NAME)
        return RunOnlyResponse(
            layout = layout,
            state = createRecipeState(newState, request.serverTime),
            analyticsInfo = AnalyticsInfo(
                intent = HowMuchToPutIntent.NAME,
                objects = listOf(
                    AnalyticsInfoRecipe(recipe),
                    AnalyticsInfoRecipeSession(state.sessionId.get(), recipe, state.currentStepId.orElse(0))
                )
            ),
            isExpectsRequest = false,
        )
    }

    fun parseSlots(request: MegaMindRequest<DialogovoState>): Slots? {
        var frame = request.getSemanticFrame(SemanticFrames.RECIPE_HOW_MUCH_TO_PUT)
        var isEllipsis = false
        if (frame == null) {
            frame = if (request.state?.recipesState?.previousIntent?.orElse(null) == HowMuchToPutIntent.NAME)
                request.getSemanticFrame(SemanticFrames.RECIPE_HOW_MUCH_TO_PUT_ELLIPSIS) else null
            isEllipsis = frame != null
        }
        if (frame == null) {
            return null
        }
        try {
            val ingredientId = frame.getSlotValue(SemanticSlotType.INGREDIENT.value) ?: ""
            val ingredient = ingredientProvider[ingredientId]
            val which = frame.getSlotValue(SemanticSlotType.RECIPE_WHICH_INGREDIENT.value) ?: ""

            return Slots(
                ingredient,
                frame.getSlotValue(SemanticSlotType.RECIPE_WHICH_INGREDIENT.value) ?: "",
                isEllipsis,
                !Strings.isEmpty(which)
            )
        } catch (entityNotFound: EntityNotFound) {
            return null
        }
    }

    private fun renderIngredientList(
        random: Random,
        ingredients: List<IngredientWithQuantity>
    ): TextWithTts {
        val ingredientTexts = ingredients.map { it.toTextWithTtsWithIngredientName() }

        val requiredItems = render(ingredientTexts, null)
        return phrases.getRandomTextWithTts("you_will_need", random, requiredItems)
    }

    data class Slots(
        val ingredient: Ingredient,
        val which: String,
        val ellipsis: Boolean,
        val showIngredientName: Boolean,
    )
}
