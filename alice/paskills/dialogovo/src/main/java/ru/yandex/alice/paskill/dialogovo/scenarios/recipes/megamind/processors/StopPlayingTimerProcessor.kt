package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.processors

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.BaseRunResponse
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.directive.MegaMindDirective
import ru.yandex.alice.kronstadt.core.directive.StopPlayingTimerDirective
import ru.yandex.alice.kronstadt.core.layout.Layout.Companion.textWithOutputSpeech
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
class StopPlayingTimerProcessor(
    private val recipeNavigator: RecipeNavigator,
    recipeProvider: RecipeProvider,
) : InsideRecipeProcessor(recipeProvider), WithNativeTimers {
    override fun canProcess(request: MegaMindRequest<DialogovoState>): Boolean {
        return isInRecipe(request) &&
            Predicates.SUPPORTS_NATIVE_TIMERS.test(request.clientInfo) &&
            findMatchingDeviceTimers(request).any(TimerState::isCurrentlyPlaying) &&
            (request.getSemanticFrame(SemanticFrames.RECIPE_STOP_PLAYING_TIMER) != null ||
                request.getSemanticFrame(SemanticFrames.RECIPE_NEXT_STEP) != null)
    }

    override fun getSemanticFrame(): String = SemanticFrames.RECIPE_STOP_PLAYING_TIMER

    override val type: RunRequestProcessorType = RunRequestProcessorType.STOP_PLAYING_TIMER

    override fun process(context: Context, request: MegaMindRequest<DialogovoState>): BaseRunResponse<DialogovoState> {
        val recipe = getRecipe(request.getStateO(), recipeProvider)
        val state = getState(request.getStateO())
        val timers = findMatchingDeviceTimers(request)
        val split = SplitTimers(timers)
        val nextStep = recipeNavigator.moveToNextStep(
            recipe,
            state,
            request.clientInfo,
            request.random,
            request.serverTime,
            request.experiments
        )
        val newState = nextStep.state.withTimers(split.notPlaying)
        val playingTimer: TimerState = split.currentlyPlaying[0]
        val stopTimers: List<MegaMindDirective> = split.currentlyPlaying
            .filter { t: TimerState -> t.deviceId.isPresent }
            .map { t: TimerState -> StopPlayingTimerDirective(t.deviceId.get()) }

        val layout = textWithOutputSpeech(
            playingTimer.textWithTts,
            nextStep.isShouldListen,
            stopTimers
        )

        return RunOnlyResponse(
            layout = layout,
            state = createRecipeState(newState, request.serverTime),
            analyticsInfo = AnalyticsInfo(
                intent = TimerStopPlaying.NAME,
                objects = listOf(
                    AnalyticsInfoRecipe(recipe), AnalyticsInfoRecipeSession(
                        nextStep.state.sessionId.get(),
                        recipe,
                        nextStep.state.currentStepId.orElse(0)
                    )
                )
            ),
            isExpectsRequest = false
        )
    }

    private data class SplitTimers(val timers: List<TimerState>) {
        val currentlyPlaying: List<TimerState>
            get() = timers.filter(TimerState::isCurrentlyPlaying)
        val notPlaying: List<TimerState>
            get() = timers.filterNot { t: TimerState -> t.isCurrentlyPlaying }
    }
}
