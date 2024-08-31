package ru.yandex.alice.kronstadt.core.convert.request

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrame
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrameSlot
import ru.yandex.alice.megamind.protos.common.FrameProto.TSemanticFrame
import ru.yandex.alice.megamind.protos.common.FrameProto.TSemanticFrame.TSlot

@Component
open class SemanticFrameConverter : FromProtoConverter<TSemanticFrame, SemanticFrame> {
    override fun convert(src: TSemanticFrame): SemanticFrame {
        val slots = src.slotsList.map { slot: TSlot ->
            SemanticFrameSlot(
                slot.name,
                slot.type,
                slot.value,
                slot.acceptedTypesList
            )
        }
        return SemanticFrame.create(src.name, slots, src.typedSemanticFrame)
    }
}
