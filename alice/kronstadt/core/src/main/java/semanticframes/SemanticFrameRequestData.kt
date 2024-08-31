package ru.yandex.alice.kronstadt.core.semanticframes

data class SemanticFrameRequestData (
    val typedSemanticFrame: TypedSemanticFrame,
    val analytics: AnalyticsTrackingModule,
) {

    data class AnalyticsTrackingModule (
        val productScenario: String,
        val purpose: String,
    )
}
