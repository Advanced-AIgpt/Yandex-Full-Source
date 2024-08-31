package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo

import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState

object DialogovoUtils {

    @JvmStatic
    fun isInSkillTab(request: MegaMindRequest<DialogovoState>, skillId: String): Boolean =
        request.getDialogIdO().isPresent && request.getDialogIdO().get() == skillId
}
