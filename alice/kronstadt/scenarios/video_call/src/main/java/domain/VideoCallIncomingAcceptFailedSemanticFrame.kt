package ru.yandex.alice.kronstadt.scenarios.video_call.domain

import ru.yandex.alice.kronstadt.core.semanticframes.TypedSemanticFrame
import ru.yandex.alice.megamind.protos.common.FrameProto
import ru.yandex.alice.megamind.protos.common.FrameProto.TVideoCallIncomingAcceptFailedSemanticFrame
import ru.yandex.alice.megamind.protos.common.FrameProto.TStringSlot
import ru.yandex.alice.megamind.protos.common.FrameProto.TTypedSemanticFrame

data class VideoCallIncomingAcceptFailedSemanticFrame(
    val callId: String,
) : TypedSemanticFrame {

    override fun writeTypedSemanticFrame(builder: TTypedSemanticFrame.Builder) {
        builder.videoCallIncomingAcceptFailedSemanticFrame = TVideoCallIncomingAcceptFailedSemanticFrame.newBuilder()
            .setProvider(
                FrameProto.TVideoCallProviderSlot.newBuilder().setEnumValue(
                    FrameProto.TVideoCallProviderSlot.EValue.Telegram
                ))
            .setCallId(TStringSlot.newBuilder().setStringValue(callId))
            .build()
    }

    override fun defaultPurpose(): String {
        return "video_call_incoming_accept_failed"
    }
}
