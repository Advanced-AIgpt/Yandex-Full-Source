package ru.yandex.alice.social.sharing.document

import ru.yandex.alice.megamind.protos.analytics.scenarios.dialogovo.Dialogovo
import ru.yandex.alice.megamind.protos.common.FrameProto

fun createExternalSkillTypedSemanticFrame(
    skillId: String,
    payload: String?,
): FrameProto.TTypedSemanticFrame {
    val externalSkillActivate = FrameProto.TExternalSkillFixedActivateSemanticFrame.newBuilder()
        .setFixedSkillId(
            FrameProto.TStringSlot.newBuilder()
                .setStringValue(skillId)
        )
        .setActivationSourceType(
            FrameProto.TActivationSourceTypeSlot.newBuilder()
                .setActivationSourceType("social_sharing")
        )
    if (!payload.isNullOrEmpty()) {
        externalSkillActivate.setPayload(FrameProto.TStringSlot.newBuilder().setStringValue(payload))
    }
    return FrameProto.TTypedSemanticFrame.newBuilder()
        .setExternalSkillFixedActivateSemanticFrame(externalSkillActivate)
        .build()
}
