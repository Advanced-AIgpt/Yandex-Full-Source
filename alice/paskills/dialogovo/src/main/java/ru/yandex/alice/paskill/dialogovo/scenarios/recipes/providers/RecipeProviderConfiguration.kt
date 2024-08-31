package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.providers

import com.fasterxml.jackson.databind.ObjectMapper
import com.fasterxml.jackson.module.kotlin.readValue
import org.springframework.context.annotation.Bean
import org.springframework.context.annotation.Configuration
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.Recipe
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.providers.JsonEntityProvider.EntityNotFound
import java.io.IOException

@Configuration
open class RecipeProviderConfiguration {

    @Throws(IOException::class, EntityNotFound::class)
    private fun readFromFile(
        resourcePath: String,
        objectMapper: ObjectMapper,
        ingredientProvider: IngredientProvider,
        kitchenEquipmentProvider: KitchenEquipmentProvider,
    ): Map<String, Recipe> {
        val entityStream = javaClass.classLoader.getResourceAsStream(resourcePath)!!
        val entityList: List<Recipe.Json> = objectMapper.readValue(entityStream)
        return entityList
            .map { jsonRecipe -> jsonRecipe.toRecipe(ingredientProvider, kitchenEquipmentProvider) }
            .filter { it.isEnabled }
            .associateBy { it.id }
    }

    @Bean("recipeProvider")
    @Throws(IOException::class, EntityNotFound::class)
    open fun recipeProvider(
        objectMapper: ObjectMapper,
        ingredientProvider: IngredientProvider,
        kitchenEquipmentProvider: KitchenEquipmentProvider,
    ): RecipeProvider {
        return RecipeProvider(
            readFromFile(
                "recipes/recipes.json",
                objectMapper,
                ingredientProvider,
                kitchenEquipmentProvider
            )
        )
    }
}
