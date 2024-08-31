package ru.yandex.alice.paskill.dialogovo.scenarios.news.processor

import ru.yandex.alice.kronstadt.core.applyarguments.ApplyArguments

data class ReadNewsApplyArguments(
    val skillId: String,
    override val applyArgs: AppmetricaCommitArgs?
) : ApplyArguments,
    WithAppmetricaCommitArgs
