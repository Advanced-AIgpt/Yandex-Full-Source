package ru.yandex.alice.paskill.dialogovo.megamind.processor

import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrame
import java.util.Optional
import java.util.function.Predicate

interface MultipleSemanticFrameRunProcessor<State> : RunRequestProcessor<State> {
    val semanticFrames: Collection<String>

    fun hasFrame(): Predicate<MegaMindRequest<State>> = Predicate { request: MegaMindRequest<State> ->
        request.hasAnySemanticFrame(semanticFrames)
    }

    fun getFirstMatchedSemanticFrame(request: MegaMindRequest<State>): Optional<SemanticFrame> {
        return request.getAnySemanticFrameO(semanticFrames)
    }
}
