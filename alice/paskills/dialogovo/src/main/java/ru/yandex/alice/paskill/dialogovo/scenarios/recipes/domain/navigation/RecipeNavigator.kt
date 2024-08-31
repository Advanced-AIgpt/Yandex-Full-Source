package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.navigation

import org.apache.logging.log4j.LogManager
import org.springframework.beans.factory.annotation.Qualifier
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.domain.ClientInfo
import ru.yandex.alice.kronstadt.core.layout.TextWithTts
import ru.yandex.alice.kronstadt.core.text.Phrases
import ru.yandex.alice.paskill.dialogovo.domain.Experiments
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.RecipeConfig
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.RecipeState
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.RecipeState.TimerState
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.Recipe
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.RecipeStep
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.RecipeStepDependency
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.StepCompletedDependency
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.TimerDependency
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.NextStepIntent
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.analyticsonly.AskForRecipeFeedbackIntent
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.analyticsonly.RecipeFinishedIntent
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.analyticsonly.WaitingForTimerIntent
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.processors.Predicates
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.nlg.DurationToString.Companion.render
import java.beans.ConstructorProperties
import java.time.Instant
import java.util.Random

@Component
class RecipeNavigator(
    @param:Qualifier("recipePhrases") private val phrases: Phrases,
    private val config: RecipeConfig
) {
    fun moveToNextStep(
        recipe: Recipe,
        state: RecipeState,
        clientInfo: ClientInfo,
        random: Random,
        currentTime: Instant,
        experiments: Set<String>
    ): NextStep {
        val nextStepPeek = peekNextStep(recipe, state, currentTime)
        var reply: TextWithTts
        val newState: RecipeState
        val shouldListen: Boolean
        val intent: String
        val musicSuggested: Boolean
        val surfaceSupportsMusic = Predicates.SUPPORTS_MUSIC.test(clientInfo)
        val currentStep = state.currentStepId.map { id: Int -> recipe.steps[id] }

        logger.info("nextStepPeek: {}", nextStepPeek)
        if (nextStepPeek is RecipeStepWithPosition) {
            val nextStep = nextStepPeek.step
            if (currentStep.isPresent && currentStep.get().shouldSetTimer() && !timerIsAlreadySet(recipe, state)) {
                logger.info("moving to next step with nonblocking timer")
                val phraseKey = if (Predicates.SUPPORTS_NATIVE_TIMERS.test(clientInfo)) {
                    "timer_set_with_next_step"
                } else {
                    "ask_user_to_set_timer_with_next_step"
                }

                reply = phrases.getRandomTextWithTts(
                    phraseKey,
                    random,
                    render(currentStep.get().getTimer().duration),
                    nextStep.toTextWithTts()
                )
                newState = state
                    .withTimer(currentTime, currentStep.get().getTimer())
                    .moveNextToStep(nextStepPeek.position)

                shouldListen = false
                intent = NextStepIntent.NAME
            } else {
                logger.info("moving to next step")
                val phraseKey = if (state.currentStepId.isEmpty) {
                    "fist_step.with_exit_suggest"
                } else {
                    "next_step"
                }
                reply = phrases.getRandomTextWithTts(phraseKey, random, nextStep.toTextWithTts())
                newState = state.moveNextToStep(nextStepPeek.position)
                shouldListen = false
                intent = NextStepIntent.NAME
            }

            if (surfaceSupportsMusic && currentStep.flatMap(RecipeStep::musicPostroll).isPresent) {
                logger.info("Surface supports music")
                val text = currentStep.get().musicPostroll.get().text

                val postrollText = text.orElse(phrases.getRandomTextWithTts("music_suggest", random))
                reply = reply.plus(postrollText)
                musicSuggested = true
            } else if (experiments.contains(Experiments.RECIPES_SUGGEST_MUSIC_AFTER_FIRST_STEP)
                || config.isSuggestMusicAfterFirstStep()
            ) {
                val postrollText = phrases.getRandomTextWithTts("music_suggest_after_first_step", random)

                reply = reply.plus(postrollText)
                musicSuggested = true
            } else {
                musicSuggested = false
            }
        } else if (nextStepPeek is WaitingForTimer) {
            if (currentStep.isPresent && currentStep.get().shouldSetTimer() && !timerIsAlreadySet(recipe, state)) {
                val duration = currentStep.get().getTimer().duration
                val timerDuration = render(currentStep.get().getTimer().duration)
                newState = state
                    .markStepAsCompleted(state.currentStepId.get())
                    .withTimer(currentTime, currentStep.get().getTimer())

                if (surfaceSupportsMusic && duration > config.getMinTimerLengthForMusicSuggest()) {
                    logger.info("setting blocking timer with music suggest")
                    reply = phrases.getRandomTextWithTts("timer_set_with_music_suggest", random, timerDuration)
                    shouldListen = true
                    musicSuggested = true
                } else {
                    logger.info("setting blocking timer")
                    val phraseKey = if (Predicates.SUPPORTS_NATIVE_TIMERS.test(
                            clientInfo
                        )
                    ) "timer_set" else "ask_user_to_set_timer"
                    reply = phrases.getRandomTextWithTts(phraseKey, random!!, timerDuration)
                    shouldListen = false
                    musicSuggested = false
                }
                intent = NextStepIntent.NAME
            } else {
                logger.info("blocked on timer")
                reply = phrases.getRandomTextWithTts("next_step_wait_please", random, render(nextStepPeek.timeLeft))
                newState = state
                shouldListen = false
                intent = WaitingForTimerIntent.NAME
                musicSuggested = false
            }
        } else if (nextStepPeek is RecipeFinished) {
            if (config.isAskForFeedback()) {
                reply = phrases.getRandomTextWithTts(
                    "recipe_done.with_reedback_request",
                    random,
                    recipe.inflectedNameCases.accusative
                )
                newState = state.askForFeedback()
                shouldListen = true
                intent = AskForRecipeFeedbackIntent.NAME
                musicSuggested = false
            } else {
                reply = phrases.getRandomTextWithTts(
                    "recipe_done",
                    random,
                    recipe.inflectedNameCases.accusative
                )
                newState = RecipeState.EMPTY
                shouldListen = false
                intent = RecipeFinishedIntent.NAME
                musicSuggested = false
            }
        } else {
            throw UnknownRecipeStepPeek()
        }
        return NextStep(reply, newState, intent, shouldListen, musicSuggested)
    }

    private fun peekNextStep(recipe: Recipe, state: RecipeState, currentTime: Instant): RecipePosition {
        val currentStepId = state.currentStepId
        val waitingForTimers: MutableList<RecipeStep> = ArrayList()
        for (position in recipe.steps.indices) {
            val step = recipe.steps[position]
            if (!state.completedSteps.contains(position) &&
                (currentStepId.isEmpty || currentStepId.get() != position)
            ) {
                if (stepIsAvailable(recipe, state, step, position)) {
                    return RecipeStepWithPosition(step, position)
                }
                val hasOnlyTimerDependencies = step.hasTimerDependencies() &&
                    step.dependencies.all { stepDependency ->
                        stepDependency.isFulfilled(recipe, state) || stepDependency is TimerDependency
                    }
                if (hasOnlyTimerDependencies) {
                    waitingForTimers.add(step)
                }
            }
        }
        if (waitingForTimers.isNotEmpty() && currentStepId.isPresent) {
            val timers: MutableList<TimerState> = ArrayList(state.timers)
            val currentStep = state.currentStepId.map { step: Int -> recipe.steps[step] }
            if (currentStep.get().hasTimer() && !timerIsAlreadySet(recipe, state)) {
                val timer = currentStep.get().getTimer()
                timers.add(
                    TimerState(
                        timer.id(currentStepId.get()),
                        currentTime,
                        timer,
                        false
                    )
                )
            }
            val timeLeft = timers.minOf { timerState: TimerState -> timerState.timeLeft(currentTime) }
            return WaitingForTimer(timeLeft)
        }
        // TODO: return RecipeFinished only if all steps are completed otherwise throw exception
        return RecipeFinished
    }

    private fun stepIsAvailable(recipe: Recipe, state: RecipeState, step: RecipeStep, stepId: Int): Boolean {
        val dependencies = if (step.dependencies.isEmpty() && stepId > 0) {
            listOf<RecipeStepDependency>(StepCompletedDependency(stepId - 1))
        } else {
            step.dependencies
        }
        return dependencies.all { dependency: RecipeStepDependency -> dependency.isFulfilled(recipe, state) }
    }

    private fun timerIsAlreadySet(recipe: Recipe, state: RecipeState): Boolean {
        val currentStep: RecipeStep? = state.currentStepId
            .map { stepId: Int -> recipe.steps[stepId] }
            ?.orElse(null)
        val timerId =
            if (currentStep?.hasTimer() == true) currentStep.getTimer().id(state.currentStepId.get()) else null
        return timerId != null && timerId in state.createdTimerIds
    }

    class NextStep @ConstructorProperties("reply", "state", "intent", "shouldListen", "hasMusicSuggest") constructor(
        val reply: TextWithTts, val state: RecipeState, val intent: String, val isShouldListen: Boolean,
        val isHasMusicSuggest: Boolean
    )

    internal class UnknownRecipeStepPeek : RuntimeException()
    companion object {
        private val logger = LogManager.getLogger()
    }
}
