package ru.yandex.alice.kronstadt.scenarios.video_call.domain

import ru.yandex.alice.kronstadt.core.semanticframes.TypedSemanticFrame
import ru.yandex.alice.megamind.protos.common.FrameProto
import ru.yandex.alice.megamind.protos.common.FrameProto.TVideoCallOutgoingAcceptedSemanticFrame
import ru.yandex.alice.megamind.protos.common.FrameProto.TTypedSemanticFrame

object VideoCallOutgoingAcceptedSemanticFrame: TypedSemanticFrame {

    override fun writeTypedSemanticFrame(builder: TTypedSemanticFrame.Builder) {
        builder.videoCallOutgoingAcceptedSemanticFrame = TVideoCallOutgoingAcceptedSemanticFrame.newBuilder()
            .setProvider(
                FrameProto.TVideoCallProviderSlot.newBuilder().setEnumValue(
                    FrameProto.TVideoCallProviderSlot.EValue.Telegram
                ))
            .build()
    }

    override fun defaultPurpose(): String {
        return "video_call_outgoing_accepted"
    }
}
