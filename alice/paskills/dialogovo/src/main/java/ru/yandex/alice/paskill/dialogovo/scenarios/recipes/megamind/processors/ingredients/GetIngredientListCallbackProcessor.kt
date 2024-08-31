package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.processors.ingredients

import org.springframework.beans.factory.annotation.Qualifier
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.BaseRunResponse
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RelevantResponse
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.analytics.analyticsInfo
import ru.yandex.alice.kronstadt.core.layout.Layout
import ru.yandex.alice.kronstadt.core.text.Phrases
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context
import ru.yandex.alice.paskill.dialogovo.megamind.processor.RunRequestProcessor
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState
import ru.yandex.alice.paskill.dialogovo.scenarios.RunRequestProcessorType
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.IngredientWithQuantity
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.entities.KitchenEquipment
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents.TellIngredientListIntent
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.analytics.AnalyticsInfoRecipe
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.processors.GetIngredientListCallbackDirective
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.processors.Predicates
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.providers.RecipeProvider
import java.util.Random

@Component
class GetIngredientListCallbackProcessor(
    private val recipeProvider: RecipeProvider,
    private val requiredListRenderer: RequiredListRenderer,
    @param:Qualifier("recipePhrases") private val phrases: Phrases
) : RunRequestProcessor<DialogovoState> {

    override fun canProcess(request: MegaMindRequest<DialogovoState>): Boolean {
        return request.input.isCallback(GetIngredientListCallbackDirective::class.java)
    }

    override val type: RunRequestProcessorType
        get() = RunRequestProcessorType.GET_INGREDIENT_LIST_CALLBACK

    override fun process(context: Context, request: MegaMindRequest<DialogovoState>): BaseRunResponse<DialogovoState> =
        render(request)

    override fun render(request: MegaMindRequest<DialogovoState>): RelevantResponse<DialogovoState> {
        val recipeId = request.input.getDirective(GetIngredientListCallbackDirective::class.java).recipeId

        val recipe = recipeProvider[recipeId]
            .orElseThrow { RuntimeException("Failed to find recipe with id $recipeId") }

        val textWithTts = requiredListRenderer.renderRequiredList(
            recipe,
            Predicates.RENDER_INGREDIENTS_AS_LIST.test(request.clientInfo),
            request.random,
            false
        )
        val layout = Layout.textLayout(
            text = textWithTts.text,
            outputSpeech = textWithTts.tts,
            shouldListen = false,
        )
        return RunOnlyResponse(
            layout = layout,
            state = request.state,
            analyticsInfo = analyticsInfo(intent = TellIngredientListIntent.analyticsInfoName) {
                obj(AnalyticsInfoRecipe(recipe))
            },
            isExpectsRequest = false,
        )
    }

    private fun renderRequiredListText(
        random: Random,
        equipmentList: List<KitchenEquipment>,
        ingredients: List<IngredientWithQuantity>
    ): String {
        val sb = StringBuilder()
            .append(phrases.getRandom("you_will_need_searchapp_callback", random))
            .append("\n")
        val prefix = "- "
        for ((_, name) in equipmentList) {
            sb.append(prefix)
            sb.append(name.tts)
            sb.append("\n")
        }
        for (ingredient in ingredients) {
            sb.append(prefix)
            sb.append(ingredient.toTextWithTtsWithIngredientName().text)
            sb.append("\n")
        }
        return sb.toString()
    }
}
