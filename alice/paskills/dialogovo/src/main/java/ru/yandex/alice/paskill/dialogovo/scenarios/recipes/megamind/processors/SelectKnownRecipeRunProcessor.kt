package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.processors

import org.apache.logging.log4j.LogManager
import ru.yandex.alice.kronstadt.core.ActionRef
import ru.yandex.alice.kronstadt.core.ActionRef.Companion.withCallback
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.layout.Layout.Companion.textWithOutputSpeech
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context
import ru.yandex.alice.paskill.dialogovo.megamind.processor.RunRequestProcessor
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState.Companion.createRecipeState
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.RecipeState
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.SelectRecipeIntent
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.analytics.AnalyticsInfoRecipe
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.analytics.AnalyticsInfoRecipeSession
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.nlg.RecipeIntroReader
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.providers.RecipeProvider
import java.util.Optional

abstract class SelectKnownRecipeRunProcessor(
    private val recipeIntroReader: RecipeIntroReader,
    private val recipeProvider: RecipeProvider
) : RunRequestProcessor<DialogovoState> {

    override fun canProcess(request: MegaMindRequest<DialogovoState>): Boolean {
        if (!Predicates.SURFACE_IS_SUPPORTED.test(request)) {
            return false
        }
        val recipeId = getRecipeId(request)
        return recipeId.flatMap { id: String -> recipeProvider[id] }.isPresent
    }

    abstract fun getRecipeId(request: MegaMindRequest<DialogovoState>): Optional<String>

    override fun process(context: Context, request: MegaMindRequest<DialogovoState>): RunOnlyResponse<DialogovoState> {
        //noinspection OptionalGetWithoutIsPresent Optional presence is guaranteed by canProcess()
        val recipe = getRecipeId(request).flatMap { id: String -> recipeProvider[id] }.get()

        context.analytics.setIntent(SelectRecipeIntent.NAME)
        val textWithTts = recipeIntroReader.readIntro(
            recipe,
            request.random,
            Predicates.RENDER_INGREDIENTS_AS_LIST.test(request.clientInfo)
        )

        val layout = textWithOutputSpeech(textWithTts, true)
        val newState = RecipeState.EMPTY.withSelectedRecipe(recipe)
        context.analytics.addObject(AnalyticsInfoRecipe(recipe))
        context.analytics.addObject(AnalyticsInfoRecipeSession(newState, recipe))
        return RunOnlyResponse(
            layout = layout,
            state = createRecipeState(newState, request.serverTime),
            analyticsInfo = context.analytics.toAnalyticsInfo(),
            isExpectsRequest = false,
            actions = VOICE_BUTTONS
        )
    }

    companion object {
        private val logger = LogManager.getLogger(SelectKnownRecipeRunProcessor::class.java)
        private val VOICE_BUTTONS = mapOf(
            "stop" to withCallback(StopCookingDirective, ActionRef.NluHints.STOP),
            "next_step" to withCallback(NextStepDirective, ActionRef.NluHints.NEXT_STEP)
        )
    }
}
