package ru.yandex.alice.paskill.dialogovo.semanticframes

import com.fasterxml.jackson.annotation.JsonInclude
import ru.yandex.alice.kronstadt.core.semanticframes.TypedSemanticFrame
import ru.yandex.alice.megamind.protos.common.FrameProto
import ru.yandex.alice.megamind.protos.common.FrameProto.TActivationSourceTypeSlot
import ru.yandex.alice.megamind.protos.common.FrameProto.TExternalSkillFixedActivateSemanticFrame
import ru.yandex.alice.megamind.protos.common.FrameProto.TTypedSemanticFrame
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType
import ru.yandex.alice.paskill.dialogovo.utils.ActivationTypedSemanticFrameUtils.createTActivationTypedSemanticFrameSlot

data class FixedActivate(
    val activationCommand: String?,
    val payload: String?,
    val skillId: String,
    val activationSourceType: ActivationSourceType,
    @JsonInclude(JsonInclude.Include.NON_NULL)
    val activationTypedSemanticFrame: TTypedSemanticFrame?
) : TypedSemanticFrame {

    constructor(
        skillId: String,
        activationSourceType: ActivationSourceType,
        activationTypedSemanticFrame: TTypedSemanticFrame? = null
    ) :
        this(
            activationCommand = null,
            payload = null,
            activationSourceType = activationSourceType,
            skillId = skillId,
            activationTypedSemanticFrame = activationTypedSemanticFrame
        )

    constructor(
        skillId: String,
        activationSourceType: ActivationSourceType,
        activationCommand: String?,
        payload: String?
    ) :
        this(
            activationCommand = activationCommand,
            payload = payload,
            activationSourceType = activationSourceType,
            skillId = skillId,
            activationTypedSemanticFrame = null
        )

    override fun writeTypedSemanticFrame(builder: TTypedSemanticFrame.Builder) {
        val fixedActivateFrameBuilder = TExternalSkillFixedActivateSemanticFrame
            .newBuilder()
            .setFixedSkillId(FrameProto.TStringSlot.newBuilder().setStringValue(skillId))

        fixedActivateFrameBuilder.setActivationSourceType(
            TActivationSourceTypeSlot.newBuilder().setActivationSourceType(activationSourceType.value())
        )

        activationCommand?.let {
            fixedActivateFrameBuilder.setActivationCommand(
                FrameProto.TStringSlot.newBuilder().setStringValue(it)
            )
        }

        activationTypedSemanticFrame?.let {
            fixedActivateFrameBuilder.setActivationTypedSemanticFrame(
                createTActivationTypedSemanticFrameSlot(activationTypedSemanticFrame)
            )
        }

        payload?.let {
            fixedActivateFrameBuilder.setPayload(
                FrameProto.TStringSlot.newBuilder().setStringValue(it)
            )
        }

        builder.externalSkillFixedActivateSemanticFrame = fixedActivateFrameBuilder.build()
    }

    override fun defaultPurpose(): String {
        return "skill_activate"
    }
}
