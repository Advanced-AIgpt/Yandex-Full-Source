package ru.yandex.alice.kronstadt.core.semanticframes

import ru.yandex.alice.megamind.protos.common.FrameProto
import ru.yandex.alice.megamind.protos.common.FrameProto.TTypedSemanticFrame

object MusicPlay : TypedSemanticFrame {
    override fun writeTypedSemanticFrame(builder: TTypedSemanticFrame.Builder) {
        builder.musicPlaySemanticFrame = FrameProto.TMusicPlaySemanticFrame.newBuilder().build()
    }

    override fun defaultPurpose(): String {
        return "music_play"
    }
}
