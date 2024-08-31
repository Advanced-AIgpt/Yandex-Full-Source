package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.processors.ingredients

import org.apache.logging.log4j.LogManager
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RelevantResponse
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.layout.Layout
import ru.yandex.alice.kronstadt.core.layout.TextWithTts
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticSlotType
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType
import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticFrames
import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticSlotEntityType
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.RepeatIntent
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.analytics.AnalyticsInfoRecipe
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.analytics.AnalyticsInfoRecipeSession
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.processors.InsideRecipeProcessor
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.processors.Predicates
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.providers.RecipeProvider

@Component
class RecipeRepeatStepProcessor(
    recipeProvider: RecipeProvider,
    private val requiredListRenderer: RequiredListRenderer
) : InsideRecipeProcessor(recipeProvider) {
    override fun getSemanticFrame(): String = SemanticFrames.RECIPE_REPEAT

    override val type: RunRequestProcessorType
        get() = RunRequestProcessorType.RECIPE_REPEAT

    override fun process(context: Context, request: MegaMindRequest<DialogovoState>): RelevantResponse<DialogovoState> =
        render(request)

    override fun render(request: MegaMindRequest<DialogovoState>): RelevantResponse<DialogovoState> {

        val recipe = getRecipe(request.getStateO(), recipeProvider)
        val state = request.state?.recipesState!!

        val stepId = state.currentStepId
        val textWithTts: TextWithTts = if (stepId.isPresent) {
            val step = recipe.steps[stepId.get()]
            step.toTextWithTts()
        } else {
            val repeatHow = request.getSemanticFrame(SemanticFrames.RECIPE_REPEAT)!!
                .getTypedEntityValueO(
                    SemanticSlotType.RECIPE_REPEAT_HOW.value,
                    SemanticSlotEntityType.RECIPE_REPETITION_MODIFIER
                )

            val usePauses = repeatHow.isPresent && repeatHow.get() == "slowly"
            requiredListRenderer.renderRequiredList(
                recipe,
                Predicates.RENDER_INGREDIENTS_AS_LIST.test(request.clientInfo),
                request.random,
                usePauses
            )
        }

        return RunOnlyResponse(
            layout = Layout.textLayout(
                text = textWithTts.text,
                outputSpeech = textWithTts.tts,
                shouldListen = false,
            ),
            state = request.state,
            analyticsInfo = AnalyticsInfo(
                RepeatIntent.NAME,
                objects = listOf(
                    AnalyticsInfoRecipe(recipe),
                    AnalyticsInfoRecipeSession(state, recipe)
                )
            ),
            isExpectsRequest = false

        )
    }

    companion object {
        private val logger = LogManager.getLogger(RecipeRepeatStepProcessor::class.java)
    }
}
