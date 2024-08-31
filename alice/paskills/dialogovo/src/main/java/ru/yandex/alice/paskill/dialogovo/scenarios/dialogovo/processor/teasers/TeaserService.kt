package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.teasers

import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessResult
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState

interface TeaserService {
    fun process(
        request: MegaMindRequest<DialogovoState>,
        skills: List<SkillInfo>,
        context: Context?,
    ) : List<SkillProcessResult>
}
