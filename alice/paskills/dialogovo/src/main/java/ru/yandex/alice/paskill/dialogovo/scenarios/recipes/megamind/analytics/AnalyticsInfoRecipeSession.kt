package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.analytics

import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoObject
import ru.yandex.alice.megamind.protos.analytics.scenarios.dialogovo.Dialogovo.TRecipeSession
import ru.yandex.alice.megamind.protos.scenarios.AnalyticsInfo.TAnalyticsInfo.TObject
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.RecipeState
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.Recipe

class AnalyticsInfoRecipeSession(id: String, val recipe: Recipe, val currentStepId: Int) :
    AnalyticsInfoObject("recipe.session", id, "Сессия приготовления") {

    // session id should always be present
    // "invalid_session_id" ids should be parsed from logs and reported as bugs
    constructor(state: RecipeState, recipe: Recipe) : this(
        id = state.sessionId.orElse("invalid_session_id"),
        recipe = recipe,
        currentStepId = state.currentStepId.orElse(0)
    )

    public override fun fillProtoField(protoBuilder: TObject.Builder): TObject.Builder {
        val payload = TRecipeSession.newBuilder()
            .setCurrentStepId(currentStepId)
        return protoBuilder.setRecipeSession(payload)
    }

    override fun toString(): String {
        return "AnalyticsInfoRecipeSession(recipe=$recipe, currentStepId=$currentStepId)"
    }
}
