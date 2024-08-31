package ru.yandex.alice.kronstadt.core.semanticframes

import ru.yandex.alice.megamind.protos.common.FrameProto.TTypedSemanticFrame
import java.util.Optional

data class SemanticFrame internal constructor(
    val name: String,
    val slots: List<SemanticFrameSlot>,
    val typedSemanticFrame: TTypedSemanticFrame? = null
) {

    operator fun get(slotName: String): SemanticFrameSlot? = getFirstSlot(slotName)

    fun getFirstSlot(slotName: String): SemanticFrameSlot? =
        slots.firstOrNull { slot -> slotName == slot.name }

    fun getSlotValue(slot: String): String? {
        return getFirstSlot(slot)?.value?.trim()?.takeIf { it.isNotEmpty() }
    }

    fun getBootSlotValue(slot: String): Boolean? {
        return getFirstSlot(slot)?.value?.equals("1")
    }

    fun `is`(type: String): Boolean {
        return name == type;
    }

    fun getSlotValueO(slot: String): Optional<String> {
        return Optional.ofNullable(getSlotValue(slot))
    }

    fun getBoolSlotValueO(slot: String): Optional<Boolean> {
        return Optional.ofNullable(getBootSlotValue(slot))
    }

    fun hasValuedSlot(slotType: String): Boolean {
        return getSlotValue(slotType) != null
    }

    fun getTypedEntityValue(
        semanticSlotType: String,
        semanticSlotEntityType: String
    ): String? {
        return slots
            .firstOrNull { slot ->
                slot.name == semanticSlotType &&
                    slot.type == semanticSlotEntityType
            }?.value
    }

    fun getTypedEntityValueO(
        semanticSlotType: String,
        semanticSlotEntityType: String
    ): Optional<String> =
        Optional.ofNullable(getTypedEntityValue(semanticSlotType, semanticSlotEntityType))

    companion object {
        @JvmStatic
        fun create(
            name: String,
            slots: List<SemanticFrameSlot>,
            typedSemanticFrame: TTypedSemanticFrame? = null
        ): SemanticFrame {
            return SemanticFrame(name, slots, typedSemanticFrame)
        }

        @JvmStatic
        fun create(
            name: String,
            typedSemanticFrame: TTypedSemanticFrame?,
            vararg slots: SemanticFrameSlot,
        ): SemanticFrame {
            return SemanticFrame(name, listOf(*slots), typedSemanticFrame)
        }

        @JvmStatic
        fun create(
            name: String,
            vararg slots: SemanticFrameSlot,
        ): SemanticFrame {
            return SemanticFrame(name, listOf(*slots))
        }
    }
}
