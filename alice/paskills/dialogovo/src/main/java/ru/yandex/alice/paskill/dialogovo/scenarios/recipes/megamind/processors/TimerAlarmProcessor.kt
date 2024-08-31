package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.processors

import org.springframework.beans.factory.annotation.Qualifier
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RelevantResponse
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.layout.Layout.Companion.textWithOutputSpeech
import ru.yandex.alice.kronstadt.core.text.Phrases
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState.Companion.createRecipeState
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType
import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticFrames
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.RecipeState.TimerState
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.navigation.RecipeNavigator
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.TimerStopPlaying
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.analytics.AnalyticsInfoRecipe
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.analytics.AnalyticsInfoRecipeSession
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.providers.RecipeProvider

@Component
class TimerAlarmProcessor(
    recipeProvider: RecipeProvider,
    private val recipeNavigator: RecipeNavigator,
    @param:Qualifier("recipePhrases") private val phrases: Phrases,
) :
    InsideRecipeProcessor(recipeProvider) {
    override fun canProcess(request: MegaMindRequest<DialogovoState>): Boolean {
        return super.canProcess(request) &&
            !Predicates.SUPPORTS_NATIVE_TIMERS.test(request.clientInfo) &&
            request.state?.recipesState?.timers?.isNotEmpty() == true
    }

    override fun getSemanticFrame(): String = SemanticFrames.RECIPE_TIMER_ALARM

    override val type: RunRequestProcessorType = RunRequestProcessorType.TIMER_ALARM

    override fun render(request: MegaMindRequest<DialogovoState>): RelevantResponse<DialogovoState> {
        val recipe = getRecipe(request.getStateO(), recipeProvider)
        val state = getState(request.getStateO())
        val timers = state.timers.sortedBy { it.timeLeft(request.serverTime).toMillis() }
        val nextStep = recipeNavigator.moveToNextStep(
            recipe,
            state,
            request.clientInfo,
            request.random,
            request.serverTime,
            request.experiments
        )

        val playingTimer = timers[0]
        val newTimers = timers.filter { t: TimerState -> t.id != playingTimer.id }

        val newState = nextStep.state.withTimers(newTimers)
        val responseTextWithTts = phrases.getRandomTextWithTts(
            "timer_move_to_next_step",
            request.random,
            playingTimer.textWithTts,
        )
        val layout = textWithOutputSpeech(responseTextWithTts, nextStep.isShouldListen)

        return RunOnlyResponse(
            layout = layout,
            state = createRecipeState(newState, request.serverTime),
            analyticsInfo = AnalyticsInfo(
                intent = TimerStopPlaying.NAME,
                objects = listOf(
                    AnalyticsInfoRecipe(recipe),
                    AnalyticsInfoRecipeSession(
                        nextStep.state.sessionId.get(),
                        recipe,
                        nextStep.state.currentStepId.orElse(0)
                    )
                )
            ),
            isExpectsRequest = false,
        )
    }

    override fun process(context: Context, request: MegaMindRequest<DialogovoState>): RelevantResponse<DialogovoState> =
        render(request)
}
