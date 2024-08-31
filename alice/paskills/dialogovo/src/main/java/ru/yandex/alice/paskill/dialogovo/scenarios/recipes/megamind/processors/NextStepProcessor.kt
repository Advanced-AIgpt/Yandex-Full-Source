package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.processors

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.ActionRef
import ru.yandex.alice.kronstadt.core.ActionRef.Companion.withCallback
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.analytics.analyticsInfo
import ru.yandex.alice.kronstadt.core.directive.MegaMindDirective
import ru.yandex.alice.kronstadt.core.directive.SetTimerDirective
import ru.yandex.alice.kronstadt.core.directive.TtsPlayPlaceholderDirective
import ru.yandex.alice.kronstadt.core.input.UtteranceInput
import ru.yandex.alice.kronstadt.core.layout.Layout.Companion.textWithOutputSpeech
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState.Companion.createRecipeState
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType
import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticFrames
import ru.yandex.alice.paskill.dialogovo.scenarios.VoiceButtonFactory
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.RecipeState
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.RecipeState.TimerState
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.navigation.RecipeNavigator
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.analyticsonly.AskForRecipeFeedbackIntent
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.analytics.AnalyticsInfoRecipe
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.analytics.AnalyticsInfoRecipeSession
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.analytics.MusicSuggestedEvent
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.processors.UtteranceComparator.Companion.equal
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.providers.RecipeProvider

@Component
class NextStepProcessor(
    private val recipeNavigator: RecipeNavigator,
    recipeProvider: RecipeProvider,
    private val voiceButtonFactory: VoiceButtonFactory
) : InsideRecipeProcessor(recipeProvider), WithNativeTimers {

    override fun canProcess(request: MegaMindRequest<DialogovoState>): Boolean {
        val utterance =
            if (request.input is UtteranceInput) (request.input as UtteranceInput).normalizedUtterance else null

        val canProcessWithSemanticFrame = super.canProcess(request) &&
            !(request.isAnyPlayerCurrentlyPlaying() && equal(utterance, "дальше"))

        val canProcessWithCallback = isInRecipe(request) && request.input.isCallback(NextStepDirective::class.java)

        return (request.state?.recipesState?.stateType == RecipeState.StateType.RECIPE_STEP) &&
            (canProcessWithSemanticFrame || canProcessWithCallback)
    }

    override fun getSemanticFrame(): String = SemanticFrames.RECIPE_NEXT_STEP

    override val type: RunRequestProcessorType
        get() = RunRequestProcessorType.RECIPE_NEXT_STEP

    override fun process(context: Context, request: MegaMindRequest<DialogovoState>): RunOnlyResponse<DialogovoState> =
        render(request)

    override fun render(request: MegaMindRequest<DialogovoState>): RunOnlyResponse<DialogovoState> {

        val recipe = getRecipe(request.getStateO(), recipeProvider)
        val state = getState(request.getStateO()).let { state ->
            if (Predicates.SUPPORTS_NATIVE_TIMERS.test(request.clientInfo)) {
                state.withTimers(findMatchingDeviceTimers(request))
            } else {
                state
            }
        }

        val nextStep = recipeNavigator.moveToNextStep(
            recipe,
            state,
            request.clientInfo,
            request.random,
            request.serverTime,
            request.experiments
        )
        val directives: List<MegaMindDirective>
        if (Predicates.SUPPORTS_NATIVE_TIMERS.test(request.clientInfo)) {
            directives = ArrayList()
            directives += nextStep.state.getNewTimers(state)
                .map { timer: TimerState -> SetTimerDirective(timer.duration) }

            if (directives.isNotEmpty()) {
                directives.add(TtsPlayPlaceholderDirective.TTS_PLAY_PLACEHOLDER_DIRECTIVE)
            }
        } else {
            directives = listOf()
        }
        val layout = textWithOutputSpeech(
            nextStep.reply,
            nextStep.isShouldListen,
            directives
        )

        val askForFeedback = AskForRecipeFeedbackIntent.NAME == nextStep.intent
        val actions: MutableMap<String, ActionRef> = if (!askForFeedback) HashMap(VOICE_BUTTONS) else HashMap()

        var musicSuggested: Boolean = false
        if (nextStep.isHasMusicSuggest) {
            actions.putAll(voiceButtonFactory.createMusicSuggest())
            musicSuggested = true
        }

        return RunOnlyResponse(
            layout = layout,
            state = createRecipeState(nextStep.state, request.serverTime),
            analyticsInfo = analyticsInfo(nextStep.intent) {
                obj(AnalyticsInfoRecipe(recipe))
                obj(AnalyticsInfoRecipeSession(nextStep.state, recipe))
                if (musicSuggested) {
                    event(MusicSuggestedEvent)
                }
            },
            isExpectsRequest = askForFeedback,
            actions = actions
        )
    }

    companion object {
        private val VOICE_BUTTONS = mapOf(
            "stop" to withCallback(StopCookingSuggestDirective, ActionRef.NluHints.STOP),
            "next_step" to withCallback(NextStepDirective, ActionRef.NluHints.NEXT_STEP)
        )
    }
}
