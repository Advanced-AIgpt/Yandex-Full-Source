package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents

import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticFrames

object TimerAlarm : RecipeIntent(SemanticFrames.RECIPE_TIMER_ALARM) {
    @JvmField
    val NAME = name
}
