package ru.yandex.alice.paskill.dialogovo.megamind.processor

import ru.yandex.alice.kronstadt.core.CommitResult
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.applyarguments.ApplyArguments
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context

interface CommittingRunProcessor<State, ApplyArgsType : ApplyArguments> : RunRequestProcessor<State> {
    val applyArgsType: Class<ApplyArgsType>
    fun commit(request: MegaMindRequest<State>, context: Context, applyArguments: ApplyArgsType): CommitResult
}
