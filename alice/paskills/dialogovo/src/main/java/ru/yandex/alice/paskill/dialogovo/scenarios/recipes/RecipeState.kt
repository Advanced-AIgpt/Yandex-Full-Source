package ru.yandex.alice.paskill.dialogovo.scenarios.recipes

import com.fasterxml.jackson.annotation.JsonCreator
import com.fasterxml.jackson.annotation.JsonInclude
import com.fasterxml.jackson.annotation.JsonProperty
import com.fasterxml.jackson.core.JsonGenerator
import com.fasterxml.jackson.databind.JsonSerializer
import com.fasterxml.jackson.databind.SerializerProvider
import com.fasterxml.jackson.databind.annotation.JsonSerialize
import org.apache.logging.log4j.LogManager
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.layout.TextWithTts
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.Recipe
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.RecipeStep
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.RecipeOnboardingIntent
import ru.yandex.alice.paskill.dialogovo.utils.UniqueList
import java.io.IOException
import java.time.Duration
import java.time.Instant
import java.util.Optional
import java.util.UUID
import kotlin.math.max

@JsonInclude(JsonInclude.Include.NON_ABSENT)
data class RecipeState(
    val sessionId: Optional<String>,
    val stateType: StateType,
    val currentRecipeId: Optional<String>,
    val currentStepId: Optional<Int>,
    val timers: List<TimerState>,
    val previousIntent: Optional<String>,
    val completedSteps: UniqueList<Int>,
    val createdTimerIds: Set<String>,
    val onboardingSeenIds: Set<String>,
) {

    fun withSelectedRecipe(recipe: Recipe): RecipeState {
        return RecipeState(
            sessionId = Optional.of(UUID.randomUUID().toString()),
            stateType = StateType.RECIPE_STEP,
            currentRecipeId = Optional.of(recipe.id),
            currentStepId = Optional.empty(),
            timers = listOf(),
            previousIntent = Optional.empty(),
            completedSteps = UniqueList.empty(),
            createdTimerIds = setOf(),
            onboardingSeenIds = setOf()
        )
    }

    fun moveNextToStep(nextStepId: Int): RecipeState {
        val newCompletedSteps = UniqueList(completedSteps)
        if (currentStepId.isPresent) {
            newCompletedSteps.add(currentStepId.get())
        }
        return RecipeState(
            sessionId = sessionId,
            stateType = StateType.RECIPE_STEP,
            currentRecipeId = currentRecipeId,
            currentStepId = Optional.of(nextStepId),
            timers = timers,
            previousIntent = Optional.empty(),
            completedSteps = newCompletedSteps,
            createdTimerIds = createdTimerIds,
            onboardingSeenIds = onboardingSeenIds
        )
    }

    fun moveToPreviousStep(): RecipeState {
        val newCompletedSteps = UniqueList(completedSteps)
        val lastStepId = completedSteps.last()
        return if (lastStepId == null) {
            RecipeState(
                sessionId = sessionId,
                stateType = StateType.RECIPE_STEP,
                currentRecipeId = currentRecipeId,
                currentStepId = Optional.empty(),
                timers = timers,
                previousIntent = Optional.empty(),
                completedSteps = UniqueList.empty(),
                createdTimerIds = createdTimerIds,
                onboardingSeenIds = onboardingSeenIds
            )
        } else {
            newCompletedSteps.remove(lastStepId)
            RecipeState(
                sessionId = sessionId,
                stateType = StateType.RECIPE_STEP,
                currentRecipeId = currentRecipeId,
                currentStepId = Optional.of(lastStepId),
                timers = timers,
                previousIntent = Optional.empty(),
                completedSteps = newCompletedSteps,
                createdTimerIds = createdTimerIds,
                onboardingSeenIds = onboardingSeenIds
            )
        }
    }

    fun markStepAsCompleted(stepId: Int): RecipeState {
        val newCompletedSteps = UniqueList(completedSteps)
        newCompletedSteps.add(stepId)
        logger.debug("marking step {} as completed. New completed steps: {}", stepId, newCompletedSteps)
        return RecipeState(
            sessionId = sessionId,
            stateType = stateType,
            currentRecipeId = currentRecipeId,
            currentStepId = currentStepId,
            timers = timers,
            previousIntent = previousIntent,
            completedSteps = newCompletedSteps,
            createdTimerIds = createdTimerIds,
            onboardingSeenIds = onboardingSeenIds
        )
    }

    /**
     * Resets currentStepId to Optional.empty() preserving all other fields
     */
    fun resetStepId(): RecipeState {
        return RecipeState(
            sessionId = sessionId,
            stateType = StateType.RECIPE_STEP,
            currentRecipeId = currentRecipeId,
            currentStepId = Optional.empty(),
            timers = timers,
            previousIntent = previousIntent,
            completedSteps = UniqueList.empty(),
            createdTimerIds = createdTimerIds,
            onboardingSeenIds = onboardingSeenIds
        )
    }

    fun withTimer(
        currentTime: Instant,
        timer: RecipeStep.Timer
    ): RecipeState {
        val timerId = timer.id(currentStepId.orElse(0))
        val newTimers: List<TimerState> = if (timers.none { t: TimerState -> t.id == timerId }) {
            timers + TimerState(timerId, currentTime, timer, false)
        } else {
            timers
        }
        return RecipeState(
            sessionId = sessionId,
            stateType = StateType.RECIPE_STEP,
            currentRecipeId = currentRecipeId,
            currentStepId = currentStepId,
            timers = newTimers,
            previousIntent = Optional.empty(),
            completedSteps = completedSteps,
            createdTimerIds = createdTimerIds + timerId,
            onboardingSeenIds = onboardingSeenIds
        )
    }

    fun withTimers(timers: List<TimerState>): RecipeState {
        return RecipeState(
            sessionId = sessionId,
            stateType = stateType,
            currentRecipeId = currentRecipeId,
            currentStepId = currentStepId,
            timers = timers,
            previousIntent = previousIntent,
            completedSteps = completedSteps,
            createdTimerIds = createdTimerIds,
            onboardingSeenIds = onboardingSeenIds
        )
    }

    fun deleteTimers(): RecipeState {
        return RecipeState(
            sessionId = sessionId,
            stateType = stateType,
            currentRecipeId = currentRecipeId,
            currentStepId = currentStepId,
            timers = emptyList(),
            previousIntent = Optional.empty(),
            completedSteps = completedSteps,
            createdTimerIds = createdTimerIds,
            onboardingSeenIds = onboardingSeenIds
        )
    }

    fun withPreviousIntent(intent: String): RecipeState {
        return RecipeState(
            sessionId = sessionId,
            stateType = stateType,
            currentRecipeId = currentRecipeId,
            currentStepId = currentStepId,
            timers = timers,
            previousIntent = Optional.of(intent),
            completedSteps = completedSteps,
            createdTimerIds = createdTimerIds,
            onboardingSeenIds = onboardingSeenIds
        )
    }

    fun withSeenOnboarding(recipes: List<Recipe>): RecipeState {
        val newSeenIds: MutableSet<String> = HashSet(onboardingSeenIds)
        for ((id) in recipes) {
            newSeenIds.add(id)
        }
        return RecipeState(
            sessionId = sessionId,
            stateType = stateType,
            currentRecipeId = currentRecipeId,
            currentStepId = currentStepId,
            timers = timers,
            previousIntent = Optional.of(RecipeOnboardingIntent.NAME),
            completedSteps = completedSteps,
            createdTimerIds = createdTimerIds,
            onboardingSeenIds = newSeenIds
        )
    }

    fun askForFeedback(): RecipeState {
        val newCompletedSteps = UniqueList(completedSteps)
        currentStepId.ifPresent { t: Int -> newCompletedSteps.add(t) }
        return RecipeState(
            sessionId = sessionId,
            stateType = StateType.WAITING_FOR_FEEDBACK,
            currentRecipeId = currentRecipeId,
            currentStepId = Optional.empty(),
            timers = emptyList(),
            previousIntent = Optional.empty(),
            completedSteps = newCompletedSteps,
            createdTimerIds = createdTimerIds,
            onboardingSeenIds = onboardingSeenIds
        )
    }

    fun hasActiveTimers(currentTime: Instant): Boolean {
        return timers.any { timerState -> timerState.isActive(currentTime) }
    }

    fun getNewTimers(previousState: RecipeState): List<TimerState> =
        (timers.toSet() - previousState.timers.toSet()).toList()

    class TimerState(
        val id: String,
        val deviceId: Optional<String>,
        val text: String,
        val tts: String,
        val shouldRingAt: Long,
        @field:JsonSerialize(using = DurationToLongSerializer::class) val duration: Duration,
        val isCurrentlyPlaying: Boolean
    ) {

        constructor(
            id: String,
            currentTime: Instant,
            timer: RecipeStep.Timer,
            currentlyPlaying: Boolean
        ) : this(
            id,
            Optional.empty<String>(),
            timer.text,
            timer.getTts(),
            currentTime.plus(timer.duration).toEpochMilli(),
            timer.duration,
            currentlyPlaying
        ) {
        }

        // Json deserializer is used in skill state deserialization
        // set currentlyPlaying always to true so that timer can always be stopped
        @JsonCreator
        constructor(
            @JsonProperty("id") id: String,
            @JsonProperty("text") text: String,
            @JsonProperty("tts") tts: String,
            @JsonProperty("shouldRingAt") shouldRingAt: Long,
            @JsonProperty("duration") duration: Long
        ) : this(id, Optional.empty<String>(), text, tts, shouldRingAt, Duration.ofSeconds(duration), true)

        fun withDeviceState(deviceStateTimer: MegaMindRequest.DeviceState.Timer): TimerState {
            val deviceTimerId = deviceStateTimer.timerId
            val newCurrentlyPlaying = deviceStateTimer.currentlyPlaying
            return TimerState(
                id,
                Optional.ofNullable(deviceTimerId),
                text,
                tts,
                shouldRingAt,
                duration,
                newCurrentlyPlaying
            )
        }

        val textWithTts: TextWithTts
            get() = TextWithTts(text, tts)

        fun isActive(currentTime: Instant): Boolean {
            return currentTime.toEpochMilli() < shouldRingAt
        }

        fun timeLeft(currentTime: Instant): Duration {
            return Duration.ofMillis(max(shouldRingAt - currentTime.toEpochMilli(), 0))
        }

        val startTimestamp: Instant
            get() = Instant.ofEpochMilli(shouldRingAt).minus(duration)

        internal class DurationToLongSerializer : JsonSerializer<Duration>() {
            @Throws(IOException::class)
            override fun serialize(
                value: Duration,
                gen: JsonGenerator,
                serializers: SerializerProvider
            ) {
                gen.writeObject(value.toSeconds())
            }
        }
    }

    enum class StateType {
        SELECT_RECIPE,
        RECIPE_STEP,
        RECIPE_STEP_AWAITS_TIMER,
        WAITING_FOR_FEEDBACK,
    }

    companion object {
        private val logger = LogManager.getLogger(RecipeState::class.java)

        @JvmField
        val EMPTY = RecipeState(
            sessionId = Optional.empty(),
            stateType = StateType.SELECT_RECIPE,
            currentRecipeId = Optional.empty(),
            currentStepId = Optional.empty(),
            timers = emptyList(),
            previousIntent = Optional.empty(),
            completedSteps = UniqueList.empty(),
            createdTimerIds = emptySet(),
            onboardingSeenIds = emptySet(),
        )
    }
}
