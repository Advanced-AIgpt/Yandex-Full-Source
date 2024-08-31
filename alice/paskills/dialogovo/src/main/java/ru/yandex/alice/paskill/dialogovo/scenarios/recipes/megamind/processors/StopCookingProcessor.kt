package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.processors

import org.springframework.beans.factory.annotation.Qualifier
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RelevantResponse
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.analytics.analyticsInfo
import ru.yandex.alice.kronstadt.core.layout.Layout.Companion.textWithOutputSpeech
import ru.yandex.alice.kronstadt.core.text.Phrases
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState.Companion.createRecipeState
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType
import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticFrames
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.RecipeState
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.StopCookingIntent
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.analytics.AnalyticsInfoRecipe
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.analytics.AnalyticsInfoRecipeSession
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.providers.RecipeProvider

@Component
class StopCookingProcessor(
    recipeProvider: RecipeProvider,
    @param:Qualifier("recipePhrases") private val phrases: Phrases
) : InsideRecipeProcessor(recipeProvider), WithFixedRecipeOverride {
    override fun getSemanticFrame(): String {
        return SemanticFrames.RECIPE_STOP_COOKING
    }

    override val type: RunRequestProcessorType
        get() = RunRequestProcessorType.STOP_COOKING

    override fun canProcess(request: MegaMindRequest<DialogovoState>): Boolean {
        return super.canProcess(request) ||
            isInRecipe(request) && request.input.isCallback(StopCookingDirective::class.java)
    }

    override fun render(request: MegaMindRequest<DialogovoState>): RelevantResponse<DialogovoState> {
        val recipe = recipeProvider.singleRandomRecipe(
            random = request.random,
            allowedIds = getAllowedRecipeIds(request),
            blacklist = setOf()
        )
        val textWithTts = phrases.getRandomTextWithTts(
            "stop_cooking",
            request.random,
            recipe.inflectedNameCases.accusative
        )
        val currentRecipe = getRecipe(request.getStateO(), recipeProvider)
        val oldState = request.state?.recipesState
        return RunOnlyResponse(
            layout = textWithOutputSpeech(textWithTts, false),
            state = createRecipeState(RecipeState.EMPTY, request.serverTime),
            analyticsInfo = analyticsInfo(StopCookingIntent.NAME) {
                obj(AnalyticsInfoRecipe(currentRecipe))
                if (oldState != null) {
                    obj(AnalyticsInfoRecipeSession(oldState, currentRecipe))
                }
            },
            isExpectsRequest = false
        )
    }

    override fun process(context: Context, request: MegaMindRequest<DialogovoState>): RelevantResponse<DialogovoState> {
        return render(request)
    }
}
