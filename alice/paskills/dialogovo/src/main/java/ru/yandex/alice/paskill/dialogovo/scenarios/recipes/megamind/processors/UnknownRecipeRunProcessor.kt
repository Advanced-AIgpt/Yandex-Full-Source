package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.processors

import org.springframework.beans.factory.annotation.Qualifier
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.IrrelevantResponse
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody
import ru.yandex.alice.kronstadt.core.analytics.analyticsInfo
import ru.yandex.alice.kronstadt.core.input.Input
import ru.yandex.alice.kronstadt.core.layout.Layout.Companion.textWithOutputSpeech
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticSlotType
import ru.yandex.alice.kronstadt.core.text.Phrases
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context
import ru.yandex.alice.paskill.dialogovo.megamind.processor.SingleSemanticFrameRunProcessor
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType
import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticFrames
import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticSlotEntityType
import ru.yandex.alice.paskill.dialogovo.scenarios.VoiceButtonFactory
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.analytics.AnalyticsInfoRecipe
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.analytics.UnknownRecipe.Companion.fromRecipeId
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.analytics.UnknownRecipe.Companion.fromWildcard
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.providers.RecipeProvider

@Component
class UnknownRecipeRunProcessor(
    @param:Qualifier("recipePhrases") private val phrases: Phrases,
    private val recipeProvider: RecipeProvider,
    private val voiceButtonFactory: VoiceButtonFactory
) : SingleSemanticFrameRunProcessor<DialogovoState>, WithFixedRecipeOverride {

    override fun canProcess(request: MegaMindRequest<DialogovoState>): Boolean {
        return if (hasFrame().and(Predicates.SURFACE_IS_SUPPORTED).test(request)) {
            request.getSemanticFrame(SemanticFrames.RECIPE_SELECT_RECIPE)!!
                .getTypedEntityValueO(SemanticSlotType.RECIPE.value, SemanticSlotEntityType.RECIPE)
                .flatMap { id: String -> recipeProvider[id] }
                .isEmpty
        } else {
            false
        }
    }

    override val type: RunRequestProcessorType = RunRequestProcessorType.UNKNOWN_RECIPE

    override fun getSemanticFrame(): String = SemanticFrames.RECIPE_SELECT_RECIPE

    override fun render(request: MegaMindRequest<DialogovoState>): IrrelevantResponse<DialogovoState> {
        val seenIds = request.state?.recipesState?.onboardingSeenIds ?: setOf()

        val randomRecipe = recipeProvider.singleRandomRecipe(
            request.random,
            getAllowedRecipeIds(request),
            seenIds
        )
        val textWithTts = phrases.getRandomTextWithTts(
            "select_recipe.error.recipe_not_found",
            request.random,
            randomRecipe.inflectedNameCases.accusative
        )
        val layout = textWithOutputSpeech(textWithTts, true)
        val recipeId = request.getSemanticFrame(SemanticFrames.RECIPE_SELECT_RECIPE)!!
            .getTypedEntityValueO(SemanticSlotType.RECIPE.value, SemanticSlotEntityType.RECIPE)

        val recipeWildcard = request.getSemanticFrame(SemanticFrames.RECIPE_SELECT_RECIPE)!!
            .getSlotValue(SemanticSlotType.RECIPE_WILDCARD.value)

        return IrrelevantResponse(
            ScenarioResponseBody(
                layout = layout,
                state = request.state,
                analyticsInfo = analyticsInfo(intent = "alice.recipes.recipe_not_found") {
                    if (recipeId.isPresent) {
                        obj(fromRecipeId(recipeId.get()))
                    } else {
                        val utterance = if (request.input is Input.Text) {
                            (request.input as Input.Text).originalUtterance
                        } else {
                            ""
                        }
                        obj(fromWildcard(recipeWildcard ?: utterance))
                    }
                    obj(AnalyticsInfoRecipe(randomRecipe))
                },
                isExpectsRequest = false,
                actions = voiceButtonFactory.createRecipeSuggest(randomRecipe)
            )
        )
    }

    override fun process(
        context: Context,
        request: MegaMindRequest<DialogovoState>
    ): IrrelevantResponse<DialogovoState> = render(request)
}
