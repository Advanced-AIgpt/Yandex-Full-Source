package ru.yandex.alice.kronstadt.core.scenario

import com.google.protobuf.Empty
import com.google.protobuf.Message
import ru.yandex.alice.kronstadt.core.ScenarioMeta
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoContext
import ru.yandex.alice.megamind.protos.scenarios.RequestProto

private val EMPTY = Empty.newBuilder().build()

abstract class AbstractNoStateScenario(
    scenarioMeta: ScenarioMeta,
    megamindRequestListeners: List<MegaMindRequestListener> = listOf(),
) : AbstractScenario<Any>(scenarioMeta, megamindRequestListeners) {
    final override fun stateToProto(state: Any, ctx: ToProtoContext): Message = EMPTY

    final override fun protoToState(request: RequestProto.TScenarioBaseRequest): Any? = null
}
