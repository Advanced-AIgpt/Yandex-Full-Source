package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.processors

import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.paskill.dialogovo.domain.Experiments
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState

internal interface WithFixedRecipeOverride {
    fun getAllowedRecipeIds(request: MegaMindRequest<DialogovoState>): Set<String> {
        return HashSet(
            request.getExperimentsWithCommaSeparatedListValues(Experiments.RECIPES_ONBOARDING_FIX_RECIPE, listOf())
        )
    }
}
