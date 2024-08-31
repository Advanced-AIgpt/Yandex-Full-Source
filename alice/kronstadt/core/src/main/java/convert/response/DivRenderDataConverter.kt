package ru.yandex.alice.kronstadt.core.convert.response

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.DivRenderData
import ru.yandex.alice.protos.api.renderer.Api.TDivRenderData

@Component
open class DivRenderDataConverter: ToProtoConverter<DivRenderData, TDivRenderData> {
    override fun convert(src: DivRenderData, ctx: ToProtoContext): TDivRenderData {
        return TDivRenderData.newBuilder()
            .setCardId(src.cardId)
            .setScenarioData(src.scenarioData)
            .build()
    }
}