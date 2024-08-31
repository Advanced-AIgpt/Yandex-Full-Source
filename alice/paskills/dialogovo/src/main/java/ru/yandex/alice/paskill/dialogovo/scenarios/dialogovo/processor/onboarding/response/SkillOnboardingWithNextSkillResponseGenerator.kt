package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.onboarding.response

import ru.yandex.alice.kronstadt.core.BaseRunResponse
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState

interface SkillOnboardingWithNextSkillResponseGenerator : SkillOnboardingStationResponseGenerator {
    fun generateResponseOnNext(
        context: Context,
        request: MegaMindRequest<DialogovoState>
    ): BaseRunResponse<DialogovoState>
}
