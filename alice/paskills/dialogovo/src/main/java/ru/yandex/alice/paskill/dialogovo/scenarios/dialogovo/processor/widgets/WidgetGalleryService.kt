package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.widgets

import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessResult
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState

interface WidgetGalleryService {
    fun process(
        request: MegaMindRequest<DialogovoState>,
        context: Context?,
        skills: List<SkillInfo>
    ): List<SkillProcessResult>
}
