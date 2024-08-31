package ru.yandex.alice.paskill.dialogovo.scenarios.recipes

import org.springframework.beans.factory.annotation.Qualifier
import org.springframework.context.annotation.Bean
import org.springframework.context.annotation.Configuration
import ru.yandex.alice.kronstadt.core.text.Phrases
import ru.yandex.alice.paskill.dialogovo.processor.DirectiveToDialogUriConverter
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.providers.RecipeProvider

@Configuration
open class RecipeServiceConfiguration {

    @Bean("recipeService")
    open fun recipeService(
        recipeProvider: RecipeProvider,
        @Qualifier("recipePhrases") phrases: Phrases,
        directiveToDialogUriConverter: DirectiveToDialogUriConverter
    ): RecipeService {

        val config = RecipeService.Config(true)
        return RecipeService(config, recipeProvider, phrases, directiveToDialogUriConverter)
    }
}
