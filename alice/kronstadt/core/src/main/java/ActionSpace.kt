package ru.yandex.alice.kronstadt.core

import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrameRequestData

data class ActionSpace(
    val effects: Map<String, Effect>,
    val nluHints: List<NluHint>
) {

    data class NluHint(
        val actionId: String,
        val semanticFrameName: String
    )

    sealed class Effect

    data class TypedSemanticFrameEffect(val semanticFrame: SemanticFrameRequestData) : Effect()
}
