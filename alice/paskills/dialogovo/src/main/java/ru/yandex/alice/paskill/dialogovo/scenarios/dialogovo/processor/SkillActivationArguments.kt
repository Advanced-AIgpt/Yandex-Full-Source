package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor

import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.megamind.protos.common.FrameProto
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState
import java.util.Optional

data class SkillActivationArguments(
    val ctx: Context,
    val baseRequest: MegaMindRequest<DialogovoState>,
    val activationSourceTypeO: Optional<ActivationSourceType>,
    val command: String? = null,
    val originalUtterance: String? = null,
    val payload: Optional<String> = Optional.empty(),
    val activationTypedSemanticFrame: FrameProto.TTypedSemanticFrame? = null
) {

    constructor(
        ctx: Context,
        baseRequest: MegaMindRequest<DialogovoState>,
        activationSourceTypeO: Optional<ActivationSourceType>
    ) : this(
        ctx = ctx,
        baseRequest = baseRequest,
        activationSourceTypeO = activationSourceTypeO,
        command = null,
        originalUtterance = null,
        payload = Optional.empty(),
        activationTypedSemanticFrame = null
    )
}
