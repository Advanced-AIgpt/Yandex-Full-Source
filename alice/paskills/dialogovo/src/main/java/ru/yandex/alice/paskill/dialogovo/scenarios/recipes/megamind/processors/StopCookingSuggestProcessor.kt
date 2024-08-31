package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.processors

import org.springframework.beans.factory.annotation.Qualifier
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.ActionRef
import ru.yandex.alice.kronstadt.core.ActionRef.Companion.withCallback
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RelevantResponse
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.layout.Layout.Companion.textWithOutputSpeech
import ru.yandex.alice.kronstadt.core.text.Phrases
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType
import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticFrames
import ru.yandex.alice.paskill.dialogovo.scenarios.SkillsSemanticFrames
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.analytics.AnalyticsInfoRecipe
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.analytics.AnalyticsInfoRecipeSession
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.processors.suggest.RespondWithSilenceDirective
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.providers.RecipeProvider

@Component
class StopCookingSuggestProcessor(
    recipeProvider: RecipeProvider,
    @param:Qualifier("recipePhrases") private val phrases: Phrases
) : InsideRecipeProcessor(recipeProvider) {
    override fun getSemanticFrame(): String = SemanticFrames.RECIPE_STOP_COOKING_SUGGEST

    override val type: RunRequestProcessorType
        get() = RunRequestProcessorType.STOP_COOKING_SUGGEST

    override fun canProcess(request: MegaMindRequest<DialogovoState>): Boolean {
        val canProcessWithSemanticFrame = super.canProcess(request) &&
            !request.isNewScenarioSession() &&
            request.deviceState?.isCurrentlyPlayingAnything != true
        val canProcessWithCallback =
            isInRecipe(request) && request.input.isCallback(StopCookingSuggestDirective::class.java)
        return canProcessWithSemanticFrame || canProcessWithCallback
    }

    override fun render(request: MegaMindRequest<DialogovoState>): RelevantResponse<DialogovoState> {
        val reply = phrases.getRandomTextWithTts("stop_cooking_suggest", request.random)
        val layout = textWithOutputSpeech(reply, request.isVoiceSession())
        val recipe = getRecipe(request.getStateO(), recipeProvider)
        val state = request.state?.recipesState!!

        return RunOnlyResponse(
            layout = layout,
            state = request.state,
            analyticsInfo = AnalyticsInfo(
                intent = "alice.recipes.stop_cooking_suggest",
                objects = listOf(
                    AnalyticsInfoRecipe(recipe),
                    AnalyticsInfoRecipeSession(state, recipe)
                )
            ),
            isExpectsRequest = false,
            actions = VOICE_BUTTONS,
        )
    }

    override fun process(context: Context, request: MegaMindRequest<DialogovoState>): RelevantResponse<DialogovoState> =
        render(request)

    companion object {
        private val VOICE_BUTTONS = mapOf(
            "confirm" to withCallback(
                StopCookingDirective,
                ActionRef.NluHint(SkillsSemanticFrames.ALICE_EXTERNAL_SKILL_SUGGEST_CONFIRM)
            ),  // TODO: use DoNothing semantic frame after VINS 77 release
            "decline" to withCallback(
                RespondWithSilenceDirective,
                ActionRef.NluHint(SkillsSemanticFrames.ALICE_EXTERNAL_SKILL_SUGGEST_DECLINE)
            )
        )
    }
}
