package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.intents

import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticFrames

object TimerStopPlaying : RecipeIntent(SemanticFrames.RECIPE_STOP_PLAYING_TIMER) {
    @JvmField
    val NAME = name
}
