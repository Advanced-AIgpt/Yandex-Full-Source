package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.processors

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.BaseRunResponse
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RelevantResponse
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.analytics.analyticsInfo
import ru.yandex.alice.kronstadt.core.directive.MegaMindDirective
import ru.yandex.alice.kronstadt.core.directive.SetTimerDirective
import ru.yandex.alice.kronstadt.core.directive.StopPlayingTimerDirective
import ru.yandex.alice.kronstadt.core.layout.Layout
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context
import ru.yandex.alice.paskill.dialogovo.megamind.processor.RunRequestProcessor
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.RecipeRequest
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.RecipeResponse
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.RecipeService
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.RecipeState
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.RecipeState.TimerState
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.RecipeIntent
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.SelectRecipeIntent
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.StopCookingIntent
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.analyticsonly.AskForRecipeFeedbackIntent
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.analyticsonly.RecipeFinishedIntent
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.analyticsonly.SaveRecipeFeedbackIntent
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.RecipeIntentFromSemanticFrameFactory
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.analytics.AnalyticsInfoRecipe
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.analytics.AnalyticsInfoRecipeSession

@Component
class LegacyRecipeServiceProcessor(
    private val intentFactory: RecipeIntentFromSemanticFrameFactory,
    private val recipeService: RecipeService,
    private val irrelevantResponseFactory: WeAreNotCookingResponse.Factory,
) : RunRequestProcessor<DialogovoState> {

    override fun canProcess(request: MegaMindRequest<DialogovoState>): Boolean {
        val hasRecipe = request.state?.recipesState?.currentRecipeId?.isPresent == true

        val intents: List<RecipeIntent> = intentFactory.convert(request.input.semanticFrames)
        val hasAnyRecipeIntent = intents.any { it !is SelectRecipeIntent } ||
            request.state
                ?.recipesState
                ?.stateType
                ?.let { RecipeState.StateType.WAITING_FOR_FEEDBACK == it }
            ?: false

        return Predicates.SURFACE_IS_SUPPORTED.test(request) &&
            hasRecipe &&
            hasAnyRecipeIntent
    }

    override val type: RunRequestProcessorType
        get() = RunRequestProcessorType.INSIDE_RECIPE

    override fun render(request: MegaMindRequest<DialogovoState>): RelevantResponse<DialogovoState>? {
        val recipeRequest = RecipeRequest.fromMegamindRequest(request, intentFactory)
        val recipeResponse = recipeService.handle(recipeRequest)

        return if (recipeResponse.relevant) {
            val directives: MutableList<MegaMindDirective> = ArrayList()
            directives.addAll(recipeResponse.state.getNewTimers(recipeRequest.state)
                .map { timerState: TimerState -> SetTimerDirective(timerState.duration) })

            if (shouldStopTimer(recipeRequest, recipeResponse)) {
                // stop all active timers
                val stopDirectives = request.deviceState?.activeTimers
                    ?.filter { it.currentlyPlaying }
                    ?.mapNotNull { it.timerId }
                    ?.map { StopPlayingTimerDirective(it) }
                    ?: listOf()
                directives += stopDirectives
            }

            val analytics = createAnalyticsInfo(recipeRequest.state, recipeResponse)

            return RunOnlyResponse(
                layout = Layout.textLayout(
                    text = recipeResponse.text,
                    outputSpeech = recipeResponse.tts.takeIf { request.voiceSession },
                    shouldListen = recipeResponse.shouldListen,
                    directives = directives,
                ),
                state = DialogovoState.createRecipeState(
                    recipeResponse.state,
                    request.serverTime
                ),
                analyticsInfo = analytics,
                isExpectsRequest = AskForRecipeFeedbackIntent.NAME == recipeResponse.analytics.intent.name,
                actions = mapOf(),
                serverDirectives = listOfNotNull(recipeResponse.pushDirective.orElse(null)),
            )
        } else {
            null
        }
    }

    override fun process(context: Context, request: MegaMindRequest<DialogovoState>): BaseRunResponse<DialogovoState> =
        render(request) ?: irrelevantResponseFactory.create(request)

    private fun shouldStopTimer(request: RecipeRequest, response: RecipeResponse): Boolean {
        return request.state.timers.size > response.state.timers.size
    }

    private fun createAnalyticsInfo(originalState: RecipeState, response: RecipeResponse): AnalyticsInfo {
        return analyticsInfo(response.analytics.intent.analyticsInfoName) {
            val recipe = response.analytics.recipe
            if (recipe.isPresent) {
                obj(AnalyticsInfoRecipe(recipe.get()))
                if (takeAnalyticsInfoFromRequest(response)) {
                    obj(
                        AnalyticsInfoRecipeSession(
                            originalState.sessionId.orElse(""),
                            recipe.get(),
                            0
                        )
                    )
                } else if (response.state.sessionId.isPresent) {
                    obj(
                        AnalyticsInfoRecipeSession(
                            response.state.sessionId.get(),
                            recipe.get(),
                            response.state.currentStepId.orElse(0)
                        )
                    )
                }
            }
        }
    }

    private fun takeAnalyticsInfoFromRequest(response: RecipeResponse): Boolean {
        val intent = response.analytics.intent
        return intent is RecipeFinishedIntent ||
            intent is SaveRecipeFeedbackIntent ||
            intent is StopCookingIntent
    }
}
