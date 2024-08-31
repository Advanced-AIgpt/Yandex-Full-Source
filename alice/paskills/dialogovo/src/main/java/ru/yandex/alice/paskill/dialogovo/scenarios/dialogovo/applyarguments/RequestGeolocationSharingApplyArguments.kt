package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.applyarguments

import ru.yandex.alice.kronstadt.core.applyarguments.ApplyArguments

data class RequestGeolocationSharingApplyArguments(
    val skillId: String,
    val normalizedUtterance: String,
) : ApplyArguments
