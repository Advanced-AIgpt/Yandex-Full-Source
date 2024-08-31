package ru.yandex.alice.paskill.dialogovo.scenarios.recipes

import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.input.Input
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.RecipeState.TimerState
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.RecipeIntent
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.RecipeIntentFromSemanticFrameFactory
import java.time.Duration
import java.time.Instant
import java.util.Optional
import java.util.Random

data class RecipeRequest(
    val utterance: String,
    val state: RecipeState,
    val intents: Map<String, RecipeIntent>,
    val isNewSession: Boolean,
    val random: Random,
    val currentTime: Instant,
    val userId: String,
    val clientInfo: RecipeClientInfo
) {

    fun <T : RecipeIntent> getIntent(expectedIntent: Class<T>): Optional<T> = Optional.ofNullable(
        intents.values
            .filterIsInstance(expectedIntent)
            .firstOrNull()
    )

    fun hasIntent(expectedIntent: Class<out RecipeIntent>): Boolean = getIntent(expectedIntent).isPresent

    fun hasIntent(name: String): Boolean = intents.containsKey(name)

    companion object {

        @JvmStatic
        fun fromMegamindRequest(
            megaMindRequest: MegaMindRequest<DialogovoState>,
            intentFactory: RecipeIntentFromSemanticFrameFactory
        ): RecipeRequest {
            var state = megaMindRequest.state?.recipesState ?: RecipeState.EMPTY
            val utterance = megaMindRequest.input.let { if (it is Input.Text) it.originalUtterance else "" }
            val deviceTimers = megaMindRequest.deviceState?.activeTimers ?: listOf()

            val recipeTimersPresentOnDevice = findMatchingDeviceTimers(state.timers, deviceTimers)

            state = state.withTimers(recipeTimersPresentOnDevice)
            val intents = intentFactory.convert(megaMindRequest.input.semanticFrames)
                .groupBy { intent -> intent.name }
                .mapValues { (_, recipeIntents) -> recipeIntents.first() }

            // use interfaces to check capabilities
            val clientInfo =
                if (megaMindRequest.clientInfo.isSmartSpeaker) RecipeClientInfo.SMART_SPEAKER else RecipeClientInfo.NOT_SMART_SPEAKER
            return RecipeRequest(
                utterance,
                state,
                intents,
                false,
                megaMindRequest.random,
                megaMindRequest.serverTime,
                megaMindRequest.clientInfo.uuid,
                clientInfo
            )
        }

        private fun findMatchingDeviceTimers(
            stateTimers: List<TimerState>,
            deviceTimers: List<MegaMindRequest.DeviceState.Timer>
        ): List<TimerState> {
            val maxDelta = Duration.ofSeconds(60)
            val result: MutableList<TimerState> = ArrayList(stateTimers.size)

            for (stateTimer in stateTimers) {
                for (deviceTimer in deviceTimers) {
                    if (Duration.between(stateTimer.startTimestamp, deviceTimer.startTimestamp).abs() < maxDelta) {
                        val isCurrentlyPlaying = deviceTimer.isCurrentlyPlaying()
                        result.add(stateTimer.withDeviceState(deviceTimer))
                    }
                }
            }
            return result
        }
    }
}
