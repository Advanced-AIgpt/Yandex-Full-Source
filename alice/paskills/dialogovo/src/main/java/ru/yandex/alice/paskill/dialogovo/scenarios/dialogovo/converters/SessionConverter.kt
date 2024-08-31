package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.converters

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoContext
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoConverter
import ru.yandex.alice.paskill.dialogovo.domain.Session
import ru.yandex.alice.paskill.dialogovo.domain.Session.ProactiveSkillExitState
import ru.yandex.alice.paskill.dialogovo.proto.DialogovoStateProto.State.ProactiveSkillExit
import ru.yandex.alice.paskill.dialogovo.proto.DialogovoStateProto.State.SkillSession

@Component
open class SessionConverter : ToProtoConverter<Session, SkillSession> {
    override fun convert(src: Session, ctx: ToProtoContext): SkillSession {
        val builder = SkillSession.newBuilder()
            .setMessageId(src.messageId)
            .setSessionId(src.sessionId)
            .setStartTimestamp(src.startTimestamp.toEpochMilli())
            .setIsEnded(src.isEnded)
            .setActivationSourceType(src.activationSourceType.type)
            .setAppMetricaEventCounter(src.appMetricaEventCounter)
            .setFailCounter(src.failCounter)
        val proactiveSkillExitState = src.proactiveSkillExitState
        // костыль для того, чтобы не переканонизировать тесты
        // TODO: убрать
        if (proactiveSkillExitState.suggestedExitAtMessageId != 0L || proactiveSkillExitState.doNotUnderstandReplyCounter != 0L) {
            builder.proactiveSkillExitState = convertProactiveSkillExitState(src.proactiveSkillExitState)
        }
        return builder.build()
    }

    private fun convertProactiveSkillExitState(src: ProactiveSkillExitState): ProactiveSkillExit {
        return ProactiveSkillExit.newBuilder()
            .setDoNotUnderstandReplyCounter(src.doNotUnderstandReplyCounter)
            .setSuggestedExitAtMessageId(src.suggestedExitAtMessageId)
            .build()
    }
}
