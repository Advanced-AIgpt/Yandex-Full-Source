package ru.yandex.alice.paskill.dialogovo.megamind

import ru.yandex.alice.kronstadt.core.CommitNeededResponse
import ru.yandex.alice.kronstadt.core.Features
import ru.yandex.alice.kronstadt.core.IScenarioResponseBody

class WrappedCommitNeededResponse<State> internal constructor(
    body: IScenarioResponseBody<State>,
    override val arguments: ApplyArgumentsWrapper,
    features: Features? = null
) : CommitNeededResponse<State>(body, arguments, features)
