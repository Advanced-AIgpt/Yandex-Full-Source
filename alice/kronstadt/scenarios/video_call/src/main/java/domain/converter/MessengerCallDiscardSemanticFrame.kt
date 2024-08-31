package ru.yandex.alice.kronstadt.scenarios.video_call.domain.converter

import ru.yandex.alice.kronstadt.core.semanticframes.TypedSemanticFrame
import ru.yandex.alice.megamind.protos.common.FrameProto
import ru.yandex.alice.megamind.protos.common.FrameProto.TTypedSemanticFrame

object MessengerCallDiscardSemanticFrame : TypedSemanticFrame {

    override fun writeTypedSemanticFrame(builder: TTypedSemanticFrame.Builder) {
        builder.messengerCallDiscardSemanticFrame = FrameProto.TMessengerCallDiscardSemanticFrame.newBuilder().build()
    }

    override fun defaultPurpose(): String {
        return "alice_scenarios.video_call_discard"
    }
}