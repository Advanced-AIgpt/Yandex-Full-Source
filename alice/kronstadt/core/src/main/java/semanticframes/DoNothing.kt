package ru.yandex.alice.kronstadt.core.semanticframes

import ru.yandex.alice.megamind.protos.common.FrameProto
import ru.yandex.alice.megamind.protos.common.FrameProto.TTypedSemanticFrame

object DoNothing : TypedSemanticFrame {
    override fun writeTypedSemanticFrame(builder: TTypedSemanticFrame.Builder) {
        builder.doNothingSemanticFrame = FrameProto.TDoNothingSemanticFrame.newBuilder().build()
    }

    override fun defaultPurpose(): String {
        return "do_nothing"
    }
}
