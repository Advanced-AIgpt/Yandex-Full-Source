package ru.yandex.alice.paskill.dialogovo.processor.discovery.activation.converter

import org.apache.logging.log4j.LogManager
import org.springframework.stereotype.Service
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.megamind.protos.common.FrameProto.TTypedSemanticFrame
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo
import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.Intent
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.directives.NewSessionDirective
import ru.yandex.alice.paskill.dialogovo.utils.ActivationTypedSemanticFrameUtils.getActivationTypedSemanticFrame

@Service
class ProvidableToSkillFrameService(
    private val providableToSkillFrameConverters: List<ProvidableToSkillFrameConverter>
) {
    private val logger = LogManager.getLogger(ProvidableToSkillFrameService::class.java)

    fun toIntent(typedSemanticFrame: TTypedSemanticFrame, skillInfo: SkillInfo): Intent? {
        val converter = providableToSkillFrameConverters.find { it.canConvert(typedSemanticFrame) }
        converter?.let {
            if (skillInfo.tags.containsAll(it.skillTag.tags))
                return it.toIntent(typedSemanticFrame)
            logger.info(
                "Converter $converter ignores providableToSkillFrame $typedSemanticFrame is not provided, " +
                    "because skill ${skillInfo.id} doesn't have required tags ${converter.skillTag.tags.joinToString()}"
            )
            return null
        }
        logger.error("Unknown type of providable to skill typed semantic frame: $typedSemanticFrame")
        return null
    }

    fun getProvidableToSkillFramesFromMMRequest(
        request: MegaMindRequest<DialogovoState>
    ): List<TTypedSemanticFrame> {
        val providableToSkillFrames: MutableList<TTypedSemanticFrame> = mutableListOf()
        if (request.input.isCallback(NewSessionDirective::class.java)) {
            val newSessionDirective = request.input.getDirective(NewSessionDirective::class.java)
            newSessionDirective.activationTypedSemanticFrame?.let { providableToSkillFrames.add(it) }
        }
        for (frame in request.input.semanticFrames) {
            val typedSemanticFrame = getActivationTypedSemanticFrame(frame)?.takeIf {
                it != TTypedSemanticFrame.getDefaultInstance()
            } ?: continue

            for (converter in providableToSkillFrameConverters) {
                if (converter.canConvert(typedSemanticFrame)) {
                    providableToSkillFrames.add(typedSemanticFrame)
                }
            }
        }
        return providableToSkillFrames
    }
}
