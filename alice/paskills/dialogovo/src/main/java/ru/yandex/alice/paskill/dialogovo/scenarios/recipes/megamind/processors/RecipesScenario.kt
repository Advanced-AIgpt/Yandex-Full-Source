package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.processors

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.input.UtteranceInput
import ru.yandex.alice.kronstadt.core.scenario.SelectedScene
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticSlotType
import ru.yandex.alice.paskill.dialogovo.megamind.AbstractProcessorBasedScenario
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoApplyArgumentsConverter
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoMegaMindRequestListener
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoStateConverter
import ru.yandex.alice.paskill.dialogovo.scenarios.Scenarios
import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticFrames
import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticSlotEntityType
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.RecipeState
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.RecipeIntent
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.SelectRecipeIntent
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.RecipeIntentFromSemanticFrameFactory
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.processors.ingredients.GetIngredientListCallbackProcessor
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.processors.ingredients.RecipeRepeatStepProcessor
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.processors.ingredients.TellIngredientListProcessor
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.processors.suggest.RespondWithSilenceDirective
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.processors.suggest.SuggestDeclineProcessor
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.providers.RecipeProvider

@Component
internal class RecipesScenario(
    applyArgumentsConverter: DialogovoApplyArgumentsConverter,
    getIngredientListCallbackProcessor: GetIngredientListCallbackProcessor,
    stopPlayingTimerProcessor: StopPlayingTimerProcessor,
    nextStepProcessor: NextStepProcessor,
    private val selectRecipeFromSemanticFrameRunProcessor: SelectRecipeFromSemanticFrameRunProcessor,
    private val selectRecipeFromCallbackRunProcessor: SelectRecipeFromCallbackRunProcessor,
    private val howMuchToPutProcessor: HowMuchToPutProcessor,
    previousStepProcessor: PreviousStepProcessor,
    tellIngredientListProcessor: TellIngredientListProcessor,
    recipeRepeatStepProcessor: RecipeRepeatStepProcessor,
    recipeOnboardingProcessor: RecipeOnboardingProcessor,
    unknownRecipeRunProcessor: UnknownRecipeRunProcessor,
    legacyRecipeServiceProcessor: LegacyRecipeServiceProcessor,
    suggestDeclineProcessor: SuggestDeclineProcessor,
    stopCookingProcessor: StopCookingProcessor,
    stopCookingSuggestProcessor: StopCookingSuggestProcessor,
    weAreNotCookingResponseFactory: WeAreNotCookingResponse.Factory,
    dialogovoStateConverter: DialogovoStateConverter,
    listener: DialogovoMegaMindRequestListener,
    private val recipeProvider: RecipeProvider,
    private val intentFactory: RecipeIntentFromSemanticFrameFactory,
) : AbstractProcessorBasedScenario<DialogovoState>(
    scenarioMeta = Scenarios.RECIPES,
    runRequestProcessors = listOf(
        getIngredientListCallbackProcessor,
        stopPlayingTimerProcessor,
        nextStepProcessor,
        selectRecipeFromSemanticFrameRunProcessor,
        selectRecipeFromCallbackRunProcessor,
        howMuchToPutProcessor,
        previousStepProcessor,
        tellIngredientListProcessor,
        recipeRepeatStepProcessor,
        recipeOnboardingProcessor,
        unknownRecipeRunProcessor,
        legacyRecipeServiceProcessor,
        suggestDeclineProcessor,
        stopCookingProcessor,
        stopCookingSuggestProcessor
    ),
    stateConverter = dialogovoStateConverter,
    applyArgumentsConverter = applyArgumentsConverter,
    irrelevantResponseFactory = weAreNotCookingResponseFactory,
    megamindRequestListeners = listOf(listener),
), WithNativeTimers {

    override fun selectSceneMigration(request: MegaMindRequest<DialogovoState>):
        SelectedScene.Running<DialogovoState, *>? = request.handle {

        onCallback<GetIngredientListCallbackDirective> {
            scene<GetIngredientListCallbackProcessor>()
        }
        onCallback<RespondWithSilenceDirective> {
            scene<SuggestDeclineProcessor>()
        }

        onCondition({ InsideRecipeProcessor.isInRecipe(recipeProvider, this) }) {

            onCondition({
                Predicates.SUPPORTS_NATIVE_TIMERS.test(clientInfo) &&
                    findMatchingDeviceTimers(this).any(RecipeState.TimerState::isCurrentlyPlaying)
            }) {
                onAnyFrame(
                    SemanticFrames.RECIPE_STOP_PLAYING_TIMER,
                    SemanticFrames.RECIPE_NEXT_STEP
                ) {
                    scene<StopPlayingTimerProcessor>()
                }
            }
            onFrame(SemanticFrames.RECIPE_TIMER_ALARM) {
                onCondition({
                    !Predicates.SUPPORTS_NATIVE_TIMERS.test(clientInfo) &&
                        state?.recipesState?.timers?.isNotEmpty() == true
                }) {
                    scene<TimerAlarmProcessor>()
                }
            }
            onCondition({ state?.recipesState?.stateType == RecipeState.StateType.RECIPE_STEP }) {
                onFrame(SemanticFrames.RECIPE_NEXT_STEP) {
                    onCondition({
                        !(isAnyPlayerCurrentlyPlaying() &&
                            UtteranceComparator.equal((input as? UtteranceInput)?.normalizedUtterance, "дальше"))
                    }) {
                        scene<NextStepProcessor>()
                    }
                }
                onCallback<NextStepDirective> {
                    scene<NextStepProcessor>()
                }
            }

            onAnyFrame(
                SemanticFrames.RECIPE_HOW_MUCH_TO_PUT,
                SemanticFrames.RECIPE_HOW_MUCH_TO_PUT_ELLIPSIS
            ) { _ ->
                onCondition({
                    howMuchToPutProcessor.parseSlots(this) != null
                }) {
                    scene<HowMuchToPutProcessor>()
                }
            }
            onFrame(SemanticFrames.RECIPE_PREVIOUS_STEP) {
                scene<PreviousStepProcessor>()
            }
            onFrame(SemanticFrames.RECIPE_TELL_INGREDIENT_LIST) {
                scene<TellIngredientListProcessor>()
            }
            onFrame(SemanticFrames.RECIPE_REPEAT) {
                scene<RecipeRepeatStepProcessor>()
            }

            onFrame(SemanticFrames.RECIPE_STOP_COOKING) {
                scene<StopCookingProcessor>()
            }
            onCallback<StopCookingDirective> {
                scene<StopCookingProcessor>()
            }
            onFrame(SemanticFrames.RECIPE_STOP_COOKING_SUGGEST) {
                onCondition(!newScenarioSession && deviceState?.isCurrentlyPlayingAnything != true) {
                    scene<StopCookingSuggestProcessor>()
                }
            }
            onCallback<StopCookingSuggestDirective> {
                scene<StopCookingSuggestProcessor>()
            }

            onCondition({

                val intents: List<RecipeIntent> = intentFactory.convert(input.semanticFrames)


                return@onCondition intents.any { it !is SelectRecipeIntent } ||
                    state?.recipesState
                        ?.stateType
                        ?.let { RecipeState.StateType.WAITING_FOR_FEEDBACK == it }
                    ?: false

            }) {
                scene<LegacyRecipeServiceProcessor>()
            }

        }
        onCondition(Predicates.SURFACE_IS_SUPPORTED) {

            onCondition({
                selectRecipeFromSemanticFrameRunProcessor.getRecipeId(this)
                    .flatMap { recipeProvider[it] }.isPresent
            }) {
                scene<SelectRecipeFromSemanticFrameRunProcessor>()
            }
            onCondition({
                selectRecipeFromCallbackRunProcessor.getRecipeId(this)
                    .flatMap { recipeProvider[it] }.isPresent
            }) {
                scene<SelectRecipeFromCallbackRunProcessor>()
            }

            onFrame(SemanticFrames.RECIPE_ONBOARDING) {
                scene<RecipeOnboardingProcessor>()
            }
            onFrameWithSlot(SemanticFrames.RECIPE_SELECT_RECIPE, SemanticSlotType.RECIPE.value) { _, slot ->
                onCondition(slot.type == SemanticSlotEntityType.RECIPE && recipeProvider[slot.value].isEmpty) {
                    scene<UnknownRecipeRunProcessor>()
                }
            }

        }
    }
}
