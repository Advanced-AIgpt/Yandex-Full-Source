package ru.yandex.alice.kronstadt.core.semanticframes

import ru.yandex.alice.megamind.protos.common.FrameProto

object SkillSessionRequest: TypedSemanticFrame {
    override fun writeTypedSemanticFrame(builder: FrameProto.TTypedSemanticFrame.Builder) {
        builder.skillSessionRequestSemanticFrame = FrameProto.TSkillSessionRequestSemanticFrame.newBuilder().build()
    }

    override fun defaultPurpose(): String {
        return "skill_session_request"
    }
}
