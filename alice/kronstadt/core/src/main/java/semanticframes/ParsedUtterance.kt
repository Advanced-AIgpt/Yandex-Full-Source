package ru.yandex.alice.kronstadt.core.semanticframes

import ru.yandex.alice.kronstadt.core.ScenarioMeta
import ru.yandex.alice.megamind.protos.common.FrameProto.TTypedSemanticFrame
import ru.yandex.alice.megamind.protos.scenarios.Frame.TParsedUtterance

data class ParsedUtterance(
    val utterance: String,
    val frame: TypedSemanticFrame,
    val buttonAnalytics: ButtonAnalytics
) {

    fun toProto(currentScenario: ScenarioMeta): TParsedUtterance {
        val frameProto = TTypedSemanticFrame.newBuilder()
        frame.writeTypedSemanticFrame(frameProto)

        return TParsedUtterance.newBuilder().apply {
            utterance = this@ParsedUtterance.utterance
            typedSemanticFrame = frameProto.build()
            analytics = buttonAnalytics.toProto(currentScenario)
        }.build()
    }
}
