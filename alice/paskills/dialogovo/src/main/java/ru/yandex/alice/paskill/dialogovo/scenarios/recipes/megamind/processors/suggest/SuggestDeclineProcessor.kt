package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.processors.suggest

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RelevantResponse
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.analytics.analyticsInfo
import ru.yandex.alice.kronstadt.core.layout.Layout
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context
import ru.yandex.alice.paskill.dialogovo.megamind.processor.RunRequestProcessor
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.RecipeState
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.analyticsonly.SuggestDeclineIntent
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.analytics.AnalyticsInfoRecipe
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.analytics.AnalyticsInfoRecipeSession
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.providers.RecipeProvider
import java.util.Optional

@Component
class SuggestDeclineProcessor(private val recipeProvider: RecipeProvider) : RunRequestProcessor<DialogovoState> {
    override fun canProcess(request: MegaMindRequest<DialogovoState>): Boolean {
        return request.input.isCallback(RespondWithSilenceDirective::class.java)
    }

    override val type: RunRequestProcessorType
        get() = RunRequestProcessorType.MUSIC_SUGGEST_DECLINE

    override fun render(request: MegaMindRequest<DialogovoState>): RelevantResponse<DialogovoState> {
        val state = Optional.ofNullable(request.state?.recipesState)
        val recipe = state.flatMap(RecipeState::currentRecipeId).flatMap { id: String -> recipeProvider[id] }
        val sessionId = state.flatMap(RecipeState::sessionId)

        val analyticsInfo = analyticsInfo(SuggestDeclineIntent.NAME) {
            if (state.isPresent && recipe.isPresent && sessionId.isPresent) {
                obj(AnalyticsInfoRecipe(recipe.get()))
                obj(
                    AnalyticsInfoRecipeSession(
                        sessionId.get(),
                        recipe.get(),
                        state.flatMap(RecipeState::currentStepId).orElse(0)
                    )
                )
            }
        }
        return RunOnlyResponse(
            layout = Layout.silence(),
            state = request.state,
            analyticsInfo = analyticsInfo,
            isExpectsRequest = false
        )
    }

    override fun process(context: Context, request: MegaMindRequest<DialogovoState>): RelevantResponse<DialogovoState> =
        render(request)
}
