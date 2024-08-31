package ru.yandex.alice.paskill.dialogovo.megamind

import ru.yandex.alice.kronstadt.core.ApplyNeededResponse
import ru.yandex.alice.kronstadt.core.Features

class WrappedApplyNeededResponse<State>
internal constructor(
    override val arguments: ApplyArgumentsWrapper,
    features: Features? = null
) : ApplyNeededResponse<State>(arguments, features)
