package ru.yandex.alice.kronstadt.scenarios.video_call.domain

import ru.yandex.alice.kronstadt.core.semanticframes.TypedSemanticFrame
import ru.yandex.alice.megamind.protos.common.FrameProto
import ru.yandex.alice.megamind.protos.common.FrameProto.TVideoCallLoginFailedSemanticFrame
import ru.yandex.alice.megamind.protos.common.FrameProto.TTypedSemanticFrame

object VideoCallLoginFailedSemanticFrame: TypedSemanticFrame {

    override fun writeTypedSemanticFrame(builder: TTypedSemanticFrame.Builder) {
        builder.videoCallLoginFailedSemanticFrame = TVideoCallLoginFailedSemanticFrame.newBuilder()
            .setProvider(
                FrameProto.TVideoCallProviderSlot.newBuilder().setEnumValue(
                    FrameProto.TVideoCallProviderSlot.EValue.Telegram
                ))
            .build()
    }

    override fun defaultPurpose(): String {
        return "video_call_login_failed"
    }
}
