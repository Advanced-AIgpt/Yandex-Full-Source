package ru.yandex.alice.kronstadt.scenarios.video_call.domain

import ru.yandex.alice.kronstadt.core.domain.Contact
import ru.yandex.alice.kronstadt.core.semanticframes.TypedSemanticFrame
import ru.yandex.alice.megamind.protos.common.FrameProto
import ru.yandex.alice.megamind.protos.common.FrameProto.TTypedSemanticFrame
import ru.yandex.alice.protos.data.scenario.video_call.VideoCall.TProviderContactData

data class VideoCallToSemanticFrame(
    val contact: Contact
) : TypedSemanticFrame {

    override fun writeTypedSemanticFrame(builder: TTypedSemanticFrame.Builder) {
        builder.videoCallToSemanticFrame = FrameProto.TVideoCallToSemanticFrame.newBuilder()
            .setFixedContact(
                FrameProto.TProviderContactSlot.newBuilder().setContactData(
                    TProviderContactData.newBuilder()
                        .setTelegramContactData(
                            TProviderContactData.TTelegramContactData
                                .newBuilder()
                                .setDisplayName(contact.displayName)
                                .setUserId(contact.contactId.toString())
                        )
                )
            ).build()
    }

    override fun defaultPurpose(): String {
        return "video_call_to"
    }
}
