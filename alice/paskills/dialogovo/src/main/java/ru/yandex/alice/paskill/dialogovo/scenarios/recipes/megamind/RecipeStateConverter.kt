package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoContext
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoConverter
import ru.yandex.alice.paskill.dialogovo.proto.DialogovoStateProto.State.TRecipeState
import ru.yandex.alice.paskill.dialogovo.proto.DialogovoStateProto.State.TRecipeState.TStateType
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.RecipeState
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.RecipeState.TimerState

@Component
open class RecipeStateConverter : ToProtoConverter<RecipeState, TRecipeState> {
    override fun convert(src: RecipeState, ctx: ToProtoContext): TRecipeState {
        val completedSteps: List<Int> = src.completedSteps.toList()
        val createdTimerIds: List<String> = src.createdTimerIds.toList().sorted()

        return TRecipeState.newBuilder()
            .setStateType(serializeRecipeStateId(src.stateType))
            .setRecipeId(src.currentRecipeId.orElse(""))
            .setCookingStarted(src.currentStepId.isPresent)
            .setCurrentStepId(src.currentStepId.orElse(0))
            .addAllTimers(src.timers.map { serializeTimer(it) })
            .setPreviousIntent(src.previousIntent.orElse(""))
            .setSessionId(src.sessionId.orElse(""))
            .addAllCompletedSteps(completedSteps)
            .addAllCreatedTimerIds(createdTimerIds)
            .addAllOnboardingSeenIds(src.onboardingSeenIds)
            .build()
    }

    private fun serializeRecipeStateId(src: RecipeState.StateType): TStateType {
        return when (src) {
            RecipeState.StateType.SELECT_RECIPE -> TStateType.SELECT_RECIPE
            RecipeState.StateType.RECIPE_STEP -> TStateType.RECIPE_STEP
            RecipeState.StateType.RECIPE_STEP_AWAITS_TIMER -> TStateType.RECIPE_STEP_AWAITS_TIMER
            RecipeState.StateType.WAITING_FOR_FEEDBACK -> TStateType.WAITING_FOR_FEEDBACK
        }
    }

    companion object {
        private fun serializeTimer(src: TimerState): TRecipeState.TTimer {
            return TRecipeState.TTimer.newBuilder()
                .setId(src.id)
                .setText(src.text)
                .setTts(src.tts)
                .setShouldRingAtEpochMs(src.shouldRingAt)
                .setDurationSeconds(src.duration.toSeconds())
                .build()
        }
    }
}
