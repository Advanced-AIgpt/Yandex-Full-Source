package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.providers

import com.fasterxml.jackson.databind.ObjectMapper
import org.springframework.stereotype.Component
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.entities.Ingredient

@Component
class IngredientProvider(objectMapper: ObjectMapper) :
    JsonEntityProvider<Ingredient>("recipes/ingredients.json", objectMapper)
