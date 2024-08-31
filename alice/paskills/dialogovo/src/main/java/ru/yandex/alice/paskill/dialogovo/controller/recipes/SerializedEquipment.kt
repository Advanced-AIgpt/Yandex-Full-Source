package ru.yandex.alice.paskill.dialogovo.controller.recipes

import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.entities.KitchenEquipment

internal data class SerializedEquipment(val text: String) {

    companion object {
        internal fun fromEquipment(equipment: KitchenEquipment) = SerializedEquipment(equipment.toTextWithTts().text)
    }
}
