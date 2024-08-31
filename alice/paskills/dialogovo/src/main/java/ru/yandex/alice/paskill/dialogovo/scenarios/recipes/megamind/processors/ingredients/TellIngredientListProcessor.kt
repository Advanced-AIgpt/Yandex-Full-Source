package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.processors.ingredients

import com.fasterxml.jackson.core.JsonProcessingException
import org.springframework.beans.factory.annotation.Qualifier
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RelevantResponse
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.directive.server.SendPushMessageDirective
import ru.yandex.alice.kronstadt.core.layout.Layout
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticSlotType
import ru.yandex.alice.kronstadt.core.text.Phrases
import ru.yandex.alice.kronstadt.core.utils.StringEnum
import ru.yandex.alice.kronstadt.core.utils.StringEnumResolver
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context
import ru.yandex.alice.paskill.dialogovo.processor.DirectiveToDialogUriConverter
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType
import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticFrames
import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticSlotEntityType
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.TellIngredientListIntent
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.analytics.AnalyticsInfoRecipe
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.analytics.AnalyticsInfoRecipeSession
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.processors.GetIngredientListCallbackDirective
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.processors.InsideRecipeProcessor
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.processors.Predicates
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.providers.RecipeProvider
import java.net.URI

@Component
class TellIngredientListProcessor(
    recipeProvider: RecipeProvider,
    private val directiveToDialogUriConverter: DirectiveToDialogUriConverter,
    private val requiredListRenderer: RequiredListRenderer,
    @param:Qualifier("recipePhrases") private val phrases: Phrases
) : InsideRecipeProcessor(recipeProvider) {
    override fun getSemanticFrame(): String = SemanticFrames.RECIPE_TELL_INGREDIENT_LIST

    override val type: RunRequestProcessorType
        get() = RunRequestProcessorType.GET_INGREDIENT_LIST

    override fun process(context: Context, request: MegaMindRequest<DialogovoState>): RelevantResponse<DialogovoState> =
        render(request)

    override fun render(request: MegaMindRequest<DialogovoState>): RelevantResponse<DialogovoState> {

        val recipe = getRecipe(request.getStateO(), recipeProvider)

        val deliveryMethod = parseDeliveryMethod(request)

        val state = request.state?.recipesState!!
        val analyticsInfo = AnalyticsInfo(
            intent = TellIngredientListIntent.NAME,
            objects = listOf(AnalyticsInfoRecipe(recipe), AnalyticsInfoRecipeSession(state, recipe)),

            )
        if (deliveryMethod == DeliveryMethod.PUSH_NOTIFICATION) {
            val textWithTts = phrases.getRandomTextWithTts("ingredient_list.sent", request.random)

            val link: URI = try {
                directiveToDialogUriConverter.convertDirectives(listOf(GetIngredientListCallbackDirective(recipe.id)))
            } catch (e: JsonProcessingException) {
                throw RuntimeException("Failed to convert directives to dialog:// URI", e)
            }

            val title = phrases.getRandom("ingredient_list_push.title", request.random, recipe.name.text)
            val text = phrases.getRandom("ingredient_list_push.text", request.random, recipe.name.text)

            val pushDirective = SendPushMessageDirective(
                title = title,
                body = text,
                link = link,
                pushId = "alice_recipes_send_ingredients_list",
                pushTag = "send-ingredients-request-" + recipe.id,
                throttlePolicy = SendPushMessageDirective.ALICE_DEFAULT_DEVICE_ID,
                appTypes = listOf(SendPushMessageDirective.AppType.SEARCH_APP),
                cardTitle = text,
                cardButtonText = "Открыть"
            )
            return RunOnlyResponse(
                layout = Layout.textLayout(
                    text = textWithTts.text,
                    outputSpeech = textWithTts.tts,
                    shouldListen = false
                ),
                state = request.state,
                analyticsInfo = analyticsInfo,
                isExpectsRequest = false,
                actions = mapOf(),
                stackEngine = null,
                serverDirectives = listOf(pushDirective)
            )
        } else {
            val textWithTts = requiredListRenderer.renderRequiredList(
                recipe,
                Predicates.RENDER_INGREDIENTS_AS_LIST.test(request.clientInfo),
                request.random,
                true
            )
            return RunOnlyResponse(
                layout = Layout.textLayout(
                    text = textWithTts.text,
                    outputSpeech = textWithTts.tts,
                    shouldListen = true,
                ),
                state = request.state,
                analyticsInfo = analyticsInfo,
                isExpectsRequest = false
            )
        }
    }

    private fun parseDeliveryMethod(request: MegaMindRequest<DialogovoState>): DeliveryMethod {
        val slotValue = request.getSemanticFrame(getSemanticFrame())
            ?.getTypedEntityValue(
                SemanticSlotType.RECIPE_DELIVERY_METHOD.value,
                SemanticSlotEntityType.RECIPE_DELIVERY_METHOD
            )

        val resolver = StringEnumResolver(DeliveryMethod::class.java)

        val deliveryMethod = slotValue?.let { value: String -> resolver.fromValueOrNull(value) }
        return deliveryMethod ?: DeliveryMethod.NONE
    }

    private enum class DeliveryMethod(private val value: String) : StringEnum {
        NONE(""), READ_SLOWLY("read_slowly"), PUSH_NOTIFICATION("push_notification");

        override fun value(): String {
            return value
        }
    }
}
