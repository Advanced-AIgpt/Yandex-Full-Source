package ru.yandex.alice.kronstadt.core.semanticframes

import ru.yandex.alice.megamind.protos.common.FrameProto.TTypedSemanticFrame

interface TypedSemanticFrame {
    fun writeTypedSemanticFrame(builder: TTypedSemanticFrame.Builder)
    fun defaultPurpose(): String
}
