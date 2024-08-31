package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.applyarguments

import ru.yandex.alice.kronstadt.core.applyarguments.ApplyArguments
import ru.yandex.alice.paskill.dialogovo.scenarios.news.processor.AppmetricaCommitArgs
import ru.yandex.alice.paskill.dialogovo.scenarios.news.processor.WithAppmetricaCommitArgs

data class AudioPlayerEventsApplyArguments(
    val skillId: String,
    override val applyArgs: AppmetricaCommitArgs?
) : ApplyArguments,
    WithAppmetricaCommitArgs
