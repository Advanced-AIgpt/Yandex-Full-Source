package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.domain.entities

import com.fasterxml.jackson.annotation.JsonCreator
import ru.yandex.alice.kronstadt.core.layout.TextWithTts
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.nlg.TtsTag
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.providers.JsonEntityProvider.EntityNotFound
import ru.yandex.alice.paskill.dialogovo.scenarios.recipes.providers.KitchenEquipmentProvider
import java.math.BigDecimal
import javax.annotation.Nonnull

data class KitchenEquipment constructor(
    override val id: String,
    override val name: TextWithTts,
    override val inflectedName: TextWithTts,
    override val pluralForms: List<TextWithTts>,
    override val ttsTag: TtsTag,
    val customText: TextWithTts?
) : GenericCountableNamedEntity {

    fun toTextWithTts(): TextWithTts = customText ?: name

    fun withCustomText(customText: TextWithTts): KitchenEquipment =
        KitchenEquipment(id, name, inflectedName, pluralForms, ttsTag, customText)

    @Nonnull
    override fun getPluralForm(number: BigDecimal): TextWithTts {
        return countableObject.getPluralForm(number)
    }

    class JsonRef(val id: String, val customText: TextWithTts?) {

        companion object {
            @JvmStatic
            @Throws(EntityNotFound::class)
            fun toKitchenEquipmentList(
                jsonRefs: List<JsonRef>,
                kitchenEquipmentProvider: KitchenEquipmentProvider
            ): List<KitchenEquipment> {
                val realKitchenEquipment: MutableList<KitchenEquipment> = ArrayList(jsonRefs.size)
                for (json in jsonRefs) {
                    var equipment = kitchenEquipmentProvider[json.id]
                    if (json.customText != null) {
                        equipment = equipment.withCustomText(json.customText)
                    }
                    realKitchenEquipment.add(equipment)
                }
                return realKitchenEquipment
            }
        }
    }

    companion object {

        @JsonCreator
        @JvmStatic
        fun fromJson(
            id: String?,
            name: String?,
            nameTts: String?,
            inflectedName: TextWithTts?,
            pluralForms: List<TextWithTts>?,
            ttsTag: TtsTag?,
            customText: TextWithTts?
        ): KitchenEquipment {
            return KitchenEquipment(
                id ?: throw RuntimeException("Kitchen equipment id cannot be null"),
                TextWithTts(name ?: throw RuntimeException("Kitchen equipment name cannot be null"), nameTts),
                inflectedName ?: throw RuntimeException("Kitchen equipment inflectedName cannot be null"),
                pluralForms ?: throw RuntimeException("Kitchen equipment pluralForms cannot be null"),
                ttsTag ?: throw RuntimeException("Kitchen equipment ttsTag cannot be null"),
                customText
            )
        }
    }
}
