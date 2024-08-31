package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.processors

import org.apache.logging.log4j.LogManager
import org.springframework.beans.factory.annotation.Qualifier
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.ActionRef
import ru.yandex.alice.kronstadt.core.ActionRef.Companion.withCallback
import ru.yandex.alice.kronstadt.core.ActionRef.Companion.withSemanticFrame
import ru.yandex.alice.kronstadt.core.BaseRunResponse
import ru.yandex.alice.kronstadt.core.IrrelevantResponse.PhrasesFactory
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RelevantResponse
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.layout.Layout.Companion.textWithOutputSpeech
import ru.yandex.alice.kronstadt.core.layout.TextWithTts
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrame.Companion.create
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticSlotType
import ru.yandex.alice.kronstadt.core.text.ListRenderer
import ru.yandex.alice.kronstadt.core.text.Phrases
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context
import ru.yandex.alice.paskill.dialogovo.megamind.processor.SingleSemanticFrameRunProcessor
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState.Companion.createRecipeState
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType
import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticFrames
import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticSlotEntityType
import ru.yandex.alice.paskill.dialogovo.scenarios.VoiceButtonFactory
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.RecipeState
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.Recipe
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.RecipeTag
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.entities.Ingredient
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.RecipeOnboardingIntent
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.analytics.AnalyticsInfoRecipe
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.providers.IngredientProvider
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.providers.RecipeProvider
import java.util.Random

@Component
class RecipeOnboardingProcessor(
    private val recipeProvider: RecipeProvider,
    private val ingredientProvider: IngredientProvider,
    @param:Qualifier("recipePhrases") private val phrases: Phrases,
    private val voiceButtonFactory: VoiceButtonFactory
) : SingleSemanticFrameRunProcessor<DialogovoState>, WithFixedRecipeOverride {
    private val irrelevantResponseFactory: NoRecipesFoundResponseFactory = NoRecipesFoundResponseFactory(phrases)

    override fun getSemanticFrame(): String = SemanticFrames.RECIPE_ONBOARDING

    override fun canProcess(request: MegaMindRequest<DialogovoState>): Boolean {
        return Predicates.SURFACE_IS_SUPPORTED
            .and(hasFrame())
            .test(request)
    }

    override val type: RunRequestProcessorType
        get() = RunRequestProcessorType.RECIPE_ONBOARDING

    override fun render(request: MegaMindRequest<DialogovoState>): RelevantResponse<DialogovoState>? {
        val tags = parseTags(request)
        val ingredients = parseIngredients(request)
        val seenIds = request.state?.recipesState?.onboardingSeenIds ?: setOf()
        val recipes: List<Recipe> = if (tags.isNotEmpty() || ingredients.isNotEmpty()) {
            recipeProvider.randomWithFilters(
                request.random,
                tags,
                ingredients,
                getAllowedRecipeIds(request),
                seenIds,
                MAX_ITEM_COUNT,
                ingredientProvider
            )
        } else {
            recipeProvider.random(request.random, getAllowedRecipeIds(request), seenIds, MAX_ITEM_COUNT)
        }
        return if (recipes.size > 1) {
            renderMultiple(request, recipes)
        } else if (recipes.size == 1) {
            renderSingle(request, recipes)
        } else {
            null
        }
    }

    override fun process(context: Context, request: MegaMindRequest<DialogovoState>): BaseRunResponse<DialogovoState> =
        render(request) ?: irrelevantResponseFactory.create(request)

    private fun renderMultiple(
        request: MegaMindRequest<DialogovoState>,
        recipes: List<Recipe>
    ): RunOnlyResponse<DialogovoState> {
        val isEllipsis = request.state
            ?.recipesState
            ?.previousIntent
            ?.orElse(null)
            ?.let { RecipeOnboardingIntent.NAME == it }
            ?: false

        val phraseKey: String
        val recipeNameGetter: (Recipe) -> TextWithTts
        if (isEllipsis) {
            phraseKey = "onboarding_more"
            recipeNameGetter = { recipe: Recipe -> recipe.inflectedNameCases.accusative }
        } else {
            phraseKey = "onboarding"
            recipeNameGetter = Recipe::name
        }
        val recipeList = ListRenderer.render(recipes.map { recipeNameGetter(it).uncapitalize() }, null)

        val textWithTts = phrases.getRandomTextWithTts(
            phraseKey,
            request.random,
            recipeList
        ).capitalize()

        val actions: MutableMap<String, ActionRef> = HashMap()
        actions += recipes
            .associate { recipe -> "select_recipe_${recipe.id}" to recipeVoiceButton(recipe) }

        actions["more"] = MORE_ACTION
        actions["select_recipe"] = SELECT_RECIPE_ELLIPSIS_ACTION

        val newState = (request.state?.recipesState ?: RecipeState.EMPTY)
            .withSeenOnboarding(recipes)
            .withPreviousIntent(RecipeOnboardingIntent.NAME)

        return RunOnlyResponse(
            layout = textWithOutputSpeech(textWithTts, true),
            state = createRecipeState(newState, request.serverTime),
            analyticsInfo = AnalyticsInfo(
                intent = RecipeOnboardingIntent.NAME,
                objects = recipes.map { AnalyticsInfoRecipe(it) }
            ),
            isExpectsRequest = false,
            actions = actions,
        )
    }

    private fun renderSingle(
        request: MegaMindRequest<DialogovoState>,
        recipes: List<Recipe>
    ): RunOnlyResponse<DialogovoState> {
        val recipe = recipes[0]
        val textWithTts = phrases.getRandomTextWithTts(
            "onboarding.1_item",
            request.random,
            recipe.name.uncapitalize()
        )

        val actions = voiceButtonFactory.createRecipeSuggest(recipe)
        val newState = (request.state?.recipesState ?: RecipeState.EMPTY).withSeenOnboarding(recipes)
        return RunOnlyResponse(
            layout = textWithOutputSpeech(textWithTts, true),
            state = createRecipeState(newState, request.serverTime),
            analyticsInfo = AnalyticsInfo(
                intent = RecipeOnboardingIntent.NAME,
                objects = listOf(AnalyticsInfoRecipe(recipe))
            ),
            isExpectsRequest = false,
            actions = actions
        )
    }

    private fun parseTags(request: MegaMindRequest<DialogovoState>): List<RecipeTag> {
        val frame = request.getSemanticFrameO(getSemanticFrame()).get()

        return listOf(SemanticSlotType.RECIPE_TAG.value, SemanticSlotType.RECIPE_TAG_2.value)
            .mapNotNull { slotType ->
                frame.getTypedEntityValue(slotType, SemanticSlotEntityType.RECIPE_TAG)
                    ?.let { tagString -> RecipeTag.STRING_ENUM_RESOLVER.fromValueOrNull(tagString) }
            }
    }

    private fun parseIngredients(request: MegaMindRequest<DialogovoState>): List<Ingredient> {
        val frame = request.getSemanticFrameO(getSemanticFrame()).get()
        return listOf(SemanticSlotType.INGREDIENT.value, SemanticSlotType.INGREDIENT_2.value)
            .mapNotNull { slotType ->
                frame.getTypedEntityValue(slotType, SemanticSlotEntityType.INGREDIENT)
                    ?.let { ingredientId -> ingredientProvider.getO(ingredientId).orElse(null) }
            }
    }

    private fun recipeVoiceButton(recipe: Recipe): ActionRef {
        val recipeName = recipe.name.uncapitalize().text
        val positives = RECIPE_VOICE_BUTTON_POSITIVES.map { String.format(it, recipeName) }

        val negatives = RECIPE_VOICE_BUTTON_NEGATIVES.map { String.format(it, recipeName).trim() } +
            RECIPE_VOICE_BUTTON_POSITIVES.map { String.format(it, "").trim() }

        return withCallback(SelectRecipeDirective(recipe.id), ActionRef.NluHint("", positives, negatives))
    }

    internal class NoRecipesFoundResponseFactory(private val phrases: Phrases) : PhrasesFactory<DialogovoState>() {

        override fun getPhrase(random: Random): TextWithTts {
            return phrases.getRandomTextWithTts( /*"onboarding.recipe_not_found"*/"we_are_not_cooking", random)
        }
    }

    companion object {
        private val logger = LogManager.getLogger()
        private const val MAX_ITEM_COUNT = 3
        private val MORE_ACTION = withSemanticFrame(
            create(SemanticFrames.RECIPE_ONBOARDING),
            ActionRef.NluHint(
                SemanticFrames.RECIPE_ONBOARDING_MORE,
                listOf(
                    "давай еще один рецепт",
                    "еще",
                    "больше",
                    "а еще один",
                    "что ещё ты умеешь готовить",
                    "какие ещё рецепты ты знаешь",
                    "какие ещё есть рецепты",
                    "давай что-нибудь другое",
                    "другой рецепт",
                    "другое блюдо",
                    "дальше",
                    "другое"
                ),
                listOf(
                    "давай",
                    "запускай",
                    "включай"
                )
            )
        )
        private val SELECT_RECIPE_ELLIPSIS_ACTION = withSemanticFrame(
            create(SemanticFrames.RECIPE_SELECT_RECIPE_ELLIPSIS),
            ActionRef.NluHint(SemanticFrames.RECIPE_SELECT_RECIPE_ELLIPSIS)
        )
        private val RECIPE_VOICE_BUTTON_POSITIVES = listOf(
            "%s",
            "давай %s",
            "включи %s",
            "запусти %s",
            "включи %s",
            "хочу %s"
        )
        private val RECIPE_VOICE_BUTTON_NEGATIVES = listOf<String>()
    }
}
