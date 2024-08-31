package ru.yandex.alice.paskill.dialogovo.megamind.processor

import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody
import ru.yandex.alice.kronstadt.core.applyarguments.ApplyArguments
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context

interface ContinuingRunProcessor<State, ApplyArgsType : ApplyArguments> : RunRequestProcessor<State> {
    val applyArgsType: Class<ApplyArgsType>
    fun processContinue(
        request: MegaMindRequest<State>, context: Context,
        applyArguments: ApplyArgsType
    ): ScenarioResponseBody<State>
}
