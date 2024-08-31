package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.processors

import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.RecipeState.TimerState
import java.time.Duration
import java.time.Instant

interface WithNativeTimers {
    fun findMatchingDeviceTimers(request: MegaMindRequest<DialogovoState>): List<TimerState> {
        val stateTimers = request.state?.recipesState?.timers ?: listOf()
        val deviceTimers = request.deviceState?.activeTimers ?: listOf()
        return findMatchingDeviceTimers(stateTimers, deviceTimers, request.serverTime)
    }

    fun findMatchingDeviceTimers(
        stateTimers: List<TimerState>,
        deviceTimers: List<MegaMindRequest.DeviceState.Timer>,
        currentTime: Instant
    ): List<TimerState> {
        val maxDelta = Duration.ofSeconds(60)
        val result = ArrayList<TimerState>(stateTimers.size)
        for (stateTimer in stateTimers) {
            for (deviceTimer in deviceTimers) {
                if (stateTimer.duration != deviceTimer.duration) {
                    continue
                }
                val timersWereCreatedAhTheSameTime = stateTimer.timeLeft(currentTime)
                    .minus(deviceTimer.remaining)
                    .abs() < maxDelta
                if (timersWereCreatedAhTheSameTime && deviceTimer.duration == stateTimer.duration) {
                    result.add(stateTimer.withDeviceState(deviceTimer))
                }
            }
        }
        return result
    }
}
