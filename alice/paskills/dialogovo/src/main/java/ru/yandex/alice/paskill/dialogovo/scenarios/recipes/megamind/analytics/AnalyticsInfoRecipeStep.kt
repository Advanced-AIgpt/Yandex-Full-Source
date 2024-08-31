package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.analytics

import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoObject

class AnalyticsInfoRecipeStep(stepId: Int) : AnalyticsInfoObject(
    "recipe.step", String.format("Recipe step #%d", stepId), String.format("Шаг рецепта №%d", stepId)
)
