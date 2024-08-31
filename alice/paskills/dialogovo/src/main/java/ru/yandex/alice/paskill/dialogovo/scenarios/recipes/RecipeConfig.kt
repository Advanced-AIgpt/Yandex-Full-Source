package ru.yandex.alice.paskill.dialogovo.scenarios.recipes

import org.springframework.boot.context.properties.ConfigurationProperties
import org.springframework.boot.context.properties.ConstructorBinding
import java.time.Duration

@ConstructorBinding
@ConfigurationProperties(prefix = "recipes")
data class RecipeConfig(
    val askForFeedback: Boolean,
    val suggestMusicAfterFirstStep: Boolean,
    private val minTimerLengthForMusicSuggest: Int,
    val slowIngredientListPauseMs: Int
) {
    fun getMinTimerLengthForMusicSuggest(): Duration = Duration.ofSeconds(minTimerLengthForMusicSuggest.toLong())
    fun isAskForFeedback() = askForFeedback
    fun isSuggestMusicAfterFirstStep() = suggestMusicAfterFirstStep
}
