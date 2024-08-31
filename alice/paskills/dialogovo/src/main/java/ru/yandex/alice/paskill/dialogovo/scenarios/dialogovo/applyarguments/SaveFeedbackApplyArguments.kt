package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.applyarguments

import ru.yandex.alice.kronstadt.core.applyarguments.ApplyArguments
import ru.yandex.alice.paskill.dialogovo.domain.FeedbackMark

data class SaveFeedbackApplyArguments(
    val mark: FeedbackMark,
    val skillId: String
) : ApplyArguments
