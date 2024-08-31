package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.processors

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RelevantResponse
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.layout.Layout.Companion.textWithOutputSpeech
import ru.yandex.alice.kronstadt.core.layout.TextWithTts
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState.Companion.createRecipeState
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType
import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticFrames
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.PreviousStepIntent
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.analytics.AnalyticsInfoRecipe
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.analytics.AnalyticsInfoRecipeSession
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.nlg.RecipeIntroReader
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.providers.RecipeProvider

@Component
class PreviousStepProcessor(recipeProvider: RecipeProvider, private val recipeIntroReader: RecipeIntroReader) :
    InsideRecipeProcessor(recipeProvider) {

    override fun getSemanticFrame(): String = SemanticFrames.RECIPE_PREVIOUS_STEP

    override val type: RunRequestProcessorType
        get() = RunRequestProcessorType.RECIPE_PREVIOUS_STEP

    override fun process(context: Context, request: MegaMindRequest<DialogovoState>): RelevantResponse<DialogovoState> =
        render(request)

    override fun render(request: MegaMindRequest<DialogovoState>): RelevantResponse<DialogovoState> {
        val recipe = getRecipe(request.getStateO(), recipeProvider)
        val state = getState(request.getStateO())
        val newState = state.moveToPreviousStep()

        val reply: TextWithTts = if (newState.currentStepId.isEmpty) {
            recipeIntroReader.readIntro(
                recipe,
                request.random,
                Predicates.RENDER_INGREDIENTS_AS_LIST.test(
                    request.clientInfo
                )
            )
        } else {
            recipe.steps[newState.currentStepId.get()].toTextWithTts()
        }

        return RunOnlyResponse(
            layout = textWithOutputSpeech(reply, false),
            state = createRecipeState(newState, request.serverTime),
            analyticsInfo = AnalyticsInfo(
                intent = PreviousStepIntent.NAME,
                objects = listOf(AnalyticsInfoRecipe(recipe), AnalyticsInfoRecipeSession(newState, recipe))
            ),
            isExpectsRequest = false
        )
    }
}
