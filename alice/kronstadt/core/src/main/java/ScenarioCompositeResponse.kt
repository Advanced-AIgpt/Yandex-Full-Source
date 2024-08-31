package ru.yandex.alice.kronstadt.core

import com.google.protobuf.Message
import ru.yandex.alice.protos.api.renderer.Api.TDivRenderData

data class ScenarioCompositeResponse<Response: Message>(
    val scenarioResponse: Response,
    val renderData: List<TDivRenderData> = listOf()
)
