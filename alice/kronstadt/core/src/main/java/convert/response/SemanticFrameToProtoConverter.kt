package ru.yandex.alice.kronstadt.core.convert.response

import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrame
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrameSlot
import ru.yandex.alice.megamind.protos.common.FrameProto.TSemanticFrame
import ru.yandex.alice.megamind.protos.common.FrameProto.TSemanticFrame.TSlot

object SemanticFrameToProtoConverter : ToProtoConverter<SemanticFrame, TSemanticFrame> {
    override fun convert(src: SemanticFrame, ctx: ToProtoContext): TSemanticFrame {
        val semanticFrameBuilder = TSemanticFrame
            .newBuilder()
            .setName(src.name)
        src.slots.forEach { slot: SemanticFrameSlot ->
            val slotBuilder = TSlot.newBuilder()
                .setName(slot.name)
                .setValue(slot.value)
                .setType(slot.type ?: "")

            slotBuilder.addAllAcceptedTypes(slot.acceptedTypes)
            semanticFrameBuilder.addSlots(slotBuilder.build())
        }
        return semanticFrameBuilder.build()
    }
}
