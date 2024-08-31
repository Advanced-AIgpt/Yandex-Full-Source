package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.providers

import com.fasterxml.jackson.databind.ObjectMapper
import org.springframework.stereotype.Component
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.entities.KitchenEquipment

@Component
class KitchenEquipmentProvider(objectMapper: ObjectMapper) :
    JsonEntityProvider<KitchenEquipment>("recipes/equipment.json", objectMapper)
