package ru.yandex.alice.paskill.dialogovo.scenarios.recipes

import com.fasterxml.jackson.core.JsonProcessingException
import org.apache.logging.log4j.LogManager
import org.springframework.beans.factory.annotation.Qualifier
import ru.yandex.alice.kronstadt.core.directive.server.SendPushMessageDirective
import ru.yandex.alice.kronstadt.core.layout.TextWithTts
import ru.yandex.alice.kronstadt.core.text.ListRenderer.render
import ru.yandex.alice.kronstadt.core.text.Phrases
import ru.yandex.alice.paskill.dialogovo.processor.DirectiveToDialogUriConverter
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.RecipeResponse.Companion.create
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.RecipeResponse.Companion.createListening
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.RecipeResponse.Companion.endSession
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.RecipeResponse.RecipeAnalytics.Companion.create
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.RecipeResponse.RecipeAnalytics.Companion.createWithoutRecipe
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.IngredientWithQuantity
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.Recipe
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.RecipeStep
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.entities.KitchenEquipment
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.DeleteStateIntent
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.HowMuchTimeWillItTakeIntent
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.NextStepIntent
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.TellEquipmentListIntent
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.TellIngredientListIntent
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.TimerAlarm
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.TimerHowMuchTimeLeft
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.TimerStopPlaying
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.analyticsonly.AskForRecipeFeedbackIntent
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.analyticsonly.RecipeFinishedIntent
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.analyticsonly.SaveRecipeFeedbackIntent
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.analyticsonly.WaitingForTimerIntent
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.analyticsonly.WelcomeIntent
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.processors.GetIngredientListCallbackDirective
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.nlg.DurationToString.Companion.render
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.providers.RecipeProvider
import java.net.URI
import java.util.Locale
import java.util.Random

class RecipeService(
    private val config: Config,
    private val recipeProvider: RecipeProvider,
    @param:Qualifier("recipePhrases") private val phrases: Phrases,
    private val directiveToDialogUriConverter: DirectiveToDialogUriConverter
) {
    fun handle(request: RecipeRequest): RecipeResponse {
        logger.info("Recipe service request: {}", request)

        if (request.hasIntent(DeleteStateIntent::class.java)) {
            return welcomeMessage(request)
        }

        val stateType = request.state.stateType
        if (stateType === RecipeState.StateType.RECIPE_STEP_AWAITS_TIMER) {
            val response = tryHandleRecipeStepAwaitsTimer(request)
            if (response != null) {
                return response
            }
        } else if (stateType === RecipeState.StateType.WAITING_FOR_FEEDBACK) {
            return saveFeedback(request)
        }
        if (request.state.timers.isNotEmpty()) {
            val response = tryHandleTimers(request)
            if (response != null) {
                return response
            }
        }
        if (stateType === RecipeState.StateType.RECIPE_STEP || stateType === RecipeState.StateType.RECIPE_STEP_AWAITS_TIMER) {
            val response = tryHandleGenericRecipeRequest(request)
            if (response != null) {
                return response
            }
        }
        val recipe = request.state.currentRecipeId.flatMap { id: String -> recipeProvider[id] }.orElse(null)
        return fallback(request, recipe)
    }

    private fun tryHandleTimers(request: RecipeRequest): RecipeResponse? {
        // currently only one timer is supported
        val state = request.state
        val timer = state.timers[0]
        val recipe = state.currentRecipeId.flatMap { id: String -> recipeProvider[id] }.get()
        val currentStepId = state.currentStepId.orElse(0)
        if ((request.hasIntent(TimerAlarm::class.java) && !request.clientInfo.supportsNativeTimers) ||
            (request.hasIntent(TimerStopPlaying::class.java) && timer.isCurrentlyPlaying)
        ) {

            return moveToNextStep(
                request,
                recipe,
                state,
                currentStepId + 1,
                "timer_move_to_next_step",
                true
            )
        } else if (request.hasIntent(TimerHowMuchTimeLeft::class.java)) {
            if (!timer.isActive(request.currentTime)) {

                return moveToNextStep(
                    request,
                    recipe,
                    state,
                    currentStepId + 1,
                    "timer_has_ended_move_to_next_step",
                    true
                )
            } else {
                val timeLeft = render(timer.timeLeft(request.currentTime))
                val textWithTts = phrases.getRandomTextWithTts(
                    "next_step_wait_please", request.random, timeLeft
                )
                val analytics = create(recipe, TimerHowMuchTimeLeft)
                return create(textWithTts, request.state, analytics)
            }
        } else {
            return null
        }
    }

    private fun tryHandleRecipeStepAwaitsTimer(request: RecipeRequest): RecipeResponse? {
        val state = request.state
        val recipe = state.currentRecipeId.flatMap { id -> recipeProvider[id] }.get()
        val currentStepId = state.currentStepId.orElse(0)
        if (request.hasIntent(NextStepIntent::class.java) || request.isNewSession) {
            val timers = state.timers
            if (timers.isNotEmpty() && timers[0].isActive(request.currentTime)) {
                val timer = timers[0]
                var textWithTts = phrases.getRandomTextWithTts(
                    "next_step_wait_please",
                    request.random,
                    render(timer.timeLeft(request.currentTime))
                )
                if (request.isNewSession) {
                    textWithTts = phrases.getRandomTextWithTts(
                        "welcome_message.continue_recipe",
                        request.random,
                        recipe.inflectedNameCases.accusative,
                        textWithTts
                    )
                }
                val analytics = create(recipe, WaitingForTimerIntent)
                return create(textWithTts, request.state, analytics)
            } else {
                return moveToNextStep(
                    request,
                    recipe,
                    state,
                    currentStepId + 1,
                    "timer_has_ended_move_to_next_step",
                    true
                )
            }
        } else {
            return null
        }
    }

    private fun tryHandleGenericRecipeRequest(request: RecipeRequest): RecipeResponse? {
        val recipe = request.state.currentRecipeId.flatMap { id: String -> recipeProvider[id] }.get()
        if (request.hasIntent(TellIngredientListIntent::class.java)) {

            return readIngredientList(request, recipe, TellIngredientListIntent)
        } else if (request.hasIntent(TellEquipmentListIntent::class.java)) {

            val textWithTts = renderEquipmentList(request.random, recipe.equipmentList)
            val analytics = create(recipe, TellEquipmentListIntent)
            return create(textWithTts, request.state, analytics)
        } else if (request.hasIntent(HowMuchTimeWillItTakeIntent::class.java)) {

            val analytics = create(recipe, HowMuchTimeWillItTakeIntent)
            return create(recipe.cookingTimeText, request.state, analytics)
        } else {
            return null
        }
    }

    private fun moveToNextStep(
        request: RecipeRequest,
        recipe: Recipe,
        origState: RecipeState,
        nextStepId: Int,
        moveToNextStepPhraseId: String,
        deleteTimers: Boolean
    ): RecipeResponse {
        var state = origState
        val nextStep: RecipeStep = try {
            recipe.steps[nextStepId]
        } catch (e: IndexOutOfBoundsException) {
            return if (config.askForFeedback) {
                val textWithTts = phrases.getRandomTextWithTts(
                    "recipe_done.with_reedback_request",
                    request.random,
                    recipe.inflectedNameCases.accusative
                )
                val newState = state.askForFeedback()
                val analytics = create(recipe, AskForRecipeFeedbackIntent)
                createListening(textWithTts, newState, analytics)
            } else {
                val analytics = create(recipe, RecipeFinishedIntent)
                endSession(
                    phrases.getRandomTextWithTts(
                        "recipe_done",
                        request.random,
                        recipe.inflectedNameCases.accusative
                    ),
                    analytics
                )
            }
        }
        val textWithTts = phrases.getRandomTextWithTts(
            moveToNextStepPhraseId,
            request.random,
            nextStep.toTextWithTts()
        )
        if (deleteTimers) {
            state = state.deleteTimers()
        }
        val analytics = create(recipe, NextStepIntent)
        return create(textWithTts, state.moveNextToStep(nextStepId), analytics)
    }

    private fun welcomeMessage(recipeRequest: RecipeRequest): RecipeResponse {
        val randomRecipeName = recipeProvider
            .singleRandomRecipe(recipeRequest.random, emptySet(), emptySet())
            .inflectedNameCases
            .accusative
            .text
            .lowercase(Locale.getDefault())
        val message = phrases.getRandom("welcome_message", recipeRequest.random, randomRecipeName)
        val analytics = createWithoutRecipe(WelcomeIntent)
        return createListening(TextWithTts(message), RecipeState.EMPTY, analytics)
    }

    private fun readIngredientList(
        request: RecipeRequest,
        recipe: Recipe,
        intent: TellIngredientListIntent
    ): RecipeResponse {
        val textWithTts: TextWithTts
        val analytics = create(recipe, intent)
        return if (request.clientInfo.isSmartSpeaker) {
            val pushDirective = createPushDirective(request.random, recipe)
            textWithTts = phrases.getRandomTextWithTts("ingredient_list.sent", request.random)
            create(textWithTts, request.state, analytics, pushDirective)
        } else {
            textWithTts = renderIngredientList(request.random, recipe.ingredients)
            create(textWithTts, request.state, analytics)
        }
    }

    private fun createPushDirective(random: Random, recipe: Recipe): SendPushMessageDirective {
        val link: URI = try {
            directiveToDialogUriConverter.convertDirectives(
                listOf(GetIngredientListCallbackDirective(recipe.id))
            )
        } catch (e: JsonProcessingException) {
            throw RuntimeException("Failed to convert directives to dialog:// URI", e)
        }
        val title = phrases.getRandom("ingredient_list_push.title", random, recipe.name.text)
        val text = phrases.getRandom("ingredient_list_push.text", random, recipe.name.text)
        return SendPushMessageDirective(
            title, text, link,
            "alice_recipes_send_ingredients_list",
            "send-ingredients-request-${recipe.id}",
            SendPushMessageDirective.ALICE_DEFAULT_DEVICE_ID,
            listOf(SendPushMessageDirective.AppType.SEARCH_APP),
            text,
            "Открыть"
        )
    }

    private fun saveFeedback(request: RecipeRequest): RecipeResponse {
        val textWithTts = phrases.getRandomTextWithTts("recipe_feedback_saved", request.random)

        val analytics = request.state.currentRecipeId.flatMap { id: String -> recipeProvider[id] }
            .map { create(it, SaveRecipeFeedbackIntent) }
            .orElseGet { createWithoutRecipe(SaveRecipeFeedbackIntent) }

        logger.info("Recipe feedback: {}", request.utterance)
        return create(textWithTts, RecipeState.EMPTY, analytics)
    }

    private fun fallback(request: RecipeRequest, textWithTts: TextWithTts, recipe: Recipe?): RecipeResponse {
        logger.warn("Detected fallback request: {}", request)
        val analytics = RecipeResponse.RecipeAnalytics.fallback(recipe)
        return RecipeResponse.fallback(textWithTts, request.state, analytics)
    }

    private fun fallback(request: RecipeRequest, recipe: Recipe?): RecipeResponse {
        val textWithTts = phrases.getRandomTextWithTts("fallback_message", request.random)
        return fallback(request, textWithTts, recipe)
    }

    private fun renderRequiredList(
        random: Random,
        equipment: List<KitchenEquipment>,
        ingredients: List<IngredientWithQuantity>
    ): TextWithTts {
        val equipmentTexts = equipment.map { it.name }
        val ingredientTexts = ingredients.map { it.toTextWithTtsWithIngredientName() }

        val requiredItems = render(equipmentTexts + ingredientTexts, null)
        return phrases.getRandomTextWithTts("you_will_need", random, requiredItems)
    }

    private fun renderIngredientList(
        random: Random,
        ingredients: List<IngredientWithQuantity>
    ): TextWithTts {
        return renderRequiredList(random, emptyList(), ingredients)
    }

    private fun renderEquipmentList(
        random: Random,
        equipment: List<KitchenEquipment>
    ): TextWithTts {
        return renderRequiredList(random, equipment, emptyList())
    }

    data class Config(val askForFeedback: Boolean)

    companion object {
        private val logger = LogManager.getLogger()
    }
}
