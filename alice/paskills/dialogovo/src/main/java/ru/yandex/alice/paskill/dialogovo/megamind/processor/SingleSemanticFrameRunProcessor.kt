package ru.yandex.alice.paskill.dialogovo.megamind.processor

import ru.yandex.alice.kronstadt.core.MegaMindRequest
import java.util.function.Predicate

interface SingleSemanticFrameRunProcessor<State> : RunRequestProcessor<State> {
    fun getSemanticFrame(): String
    fun hasFrame(): Predicate<MegaMindRequest<State>> =
        Predicate { request: MegaMindRequest<State> -> request.hasSemanticFrame(getSemanticFrame()) }
}
