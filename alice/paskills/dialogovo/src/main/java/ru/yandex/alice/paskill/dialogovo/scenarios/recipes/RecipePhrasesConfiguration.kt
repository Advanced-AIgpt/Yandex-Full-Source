package ru.yandex.alice.paskill.dialogovo.scenarios.recipes

import org.springframework.context.annotation.Bean
import org.springframework.context.annotation.Configuration
import ru.yandex.alice.kronstadt.core.text.Phrases

@Configuration
open class RecipePhrasesConfiguration {
    @Bean("recipePhrases")
    open fun recipePhrases(): Phrases = Phrases(listOf("recipes/phrases/recipes"))
}
