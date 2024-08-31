package ru.yandex.alice.paskill.dialogovo.utils

import org.apache.logging.log4j.LogManager
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrame
import ru.yandex.alice.megamind.protos.common.FrameProto
import ru.yandex.alice.megamind.protos.common.FrameProto.TTypedSemanticFrame

object ActivationTypedSemanticFrameUtils {
    private val logger = LogManager.getLogger(ActivationTypedSemanticFrameUtils::class)

    @JvmStatic
    fun getActivationTypedSemanticFrame(
        semanticFrame: SemanticFrame
    ): TTypedSemanticFrame? =
        semanticFrame.typedSemanticFrame
            ?.externalSkillFixedActivateSemanticFrame?.takeIf { it.hasActivationTypedSemanticFrame() }
            ?.activationTypedSemanticFrame?.let {
                TTypedSemanticFrame.newBuilder().apply {
                    when {
                        it.hasPutMoneyOnPhoneSemanticFrame() ->
                            putMoneyOnPhoneSemanticFrame = it.putMoneyOnPhoneSemanticFrame
                        else -> return null
                    }
                }.build()
            }

    @JvmStatic
    fun createTActivationTypedSemanticFrameSlot(it: TTypedSemanticFrame): FrameProto.TActivationTypedSemanticFrameSlot =
        FrameProto.TActivationTypedSemanticFrameSlot.newBuilder().apply {
            when {
                it.hasPutMoneyOnPhoneSemanticFrame() ->
                    putMoneyOnPhoneSemanticFrame = it.putMoneyOnPhoneSemanticFrame
                else ->
                    logger.error("Unknown type of activation typed semantic frame: $it")
            }
        }.build()
}
