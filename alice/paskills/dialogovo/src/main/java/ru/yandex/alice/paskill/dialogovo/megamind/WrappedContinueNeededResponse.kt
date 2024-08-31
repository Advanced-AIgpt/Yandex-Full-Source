package ru.yandex.alice.paskill.dialogovo.megamind

import ru.yandex.alice.kronstadt.core.ContinueNeededResponse
import ru.yandex.alice.kronstadt.core.Features

class WrappedContinueNeededResponse<State>
internal constructor(
    override val arguments: ApplyArgumentsWrapper,
    features: Features? = null
) : ContinueNeededResponse<State>(arguments, features)
