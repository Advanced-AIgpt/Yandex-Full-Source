package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.applyarguments

import com.fasterxml.jackson.annotation.JsonIgnore
import ru.yandex.alice.kronstadt.core.applyarguments.ApplyArguments
import ru.yandex.alice.megamind.protos.common.FrameProto.TActivationTypedSemanticFrameSlot
import ru.yandex.alice.megamind.protos.common.FrameProto.TTypedSemanticFrame
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType
import ru.yandex.alice.paskill.dialogovo.utils.ActivationTypedSemanticFrameUtils.createTActivationTypedSemanticFrameSlot
import java.util.Optional

data class RequestSkillApplyArguments(
    val skillId: String,
    val request: String?,
    val originalUtterance: String?,
    val activationSourceType: ActivationSourceType?,
    val resumeSessionAfterPlayerStopRequests: Int = 0,
    val activation: Boolean = false,
    val payload: String? = null,
    val activationTypedSemanticFrameSlot: TActivationTypedSemanticFrameSlot? = null
) : ApplyArguments {

    @JsonIgnore
    fun getActivationSourceTypeO() = Optional.ofNullable(activationSourceType)

    @JsonIgnore
    fun getRequestO() = Optional.ofNullable(request)

    @JsonIgnore
    fun getOriginalUtteranceO() = Optional.ofNullable(originalUtterance)

    @JsonIgnore
    fun isActivation() = activation

    companion object {
        @JvmStatic
        fun onActivation(
            skillId: String,
            request: Optional<String>,
            originalUtterance: Optional<String>,
            activationSourceType: Optional<ActivationSourceType>,
            payload: Optional<String>,
            activationTypedSemanticFrame: TTypedSemanticFrame?,
        ): RequestSkillApplyArguments {
            return RequestSkillApplyArguments(
                skillId = skillId,
                request = request.orElse(null),
                originalUtterance = originalUtterance.orElse(null),
                activationSourceType = activationSourceType.orElse(null),
                activation = true,
                payload = payload.orElse(null),
                activationTypedSemanticFrameSlot = activationTypedSemanticFrame?.let {
                    createTActivationTypedSemanticFrameSlot(activationTypedSemanticFrame)
                }
            )
        }

        @JvmStatic
        fun create(
            skillId: String,
            request: Optional<String>,
            originalUtterance: Optional<String>,
            activationSourceType: Optional<ActivationSourceType>
        ): RequestSkillApplyArguments {
            return RequestSkillApplyArguments(
                skillId = skillId,
                request = request.orElse(null),
                originalUtterance = originalUtterance.orElse(null),
                activationSourceType = activationSourceType.orElse(null)
            )
        }

        @JvmStatic
        fun create(
            skillId: String,
            request: Optional<String>,
            originalUtterance: Optional<String>,
            activationSourceType: Optional<ActivationSourceType>,
            resumeSessionAfterPlayerStopRequests: Int
        ): RequestSkillApplyArguments {
            return RequestSkillApplyArguments(
                skillId = skillId,
                request = request.orElse(null),
                originalUtterance = originalUtterance.orElse(null),
                activationSourceType = activationSourceType.orElse(null),
                resumeSessionAfterPlayerStopRequests = resumeSessionAfterPlayerStopRequests
            )
        }
    }
}
