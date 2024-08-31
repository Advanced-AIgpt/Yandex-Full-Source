package ru.yandex.alice.kronstadt.core.directive.server

import com.google.protobuf.Any
import com.google.protobuf.GeneratedMessageV3
import ru.yandex.alice.memento.proto.MementoApiProto.EConfigKey

data class UserConfig(
    val message: GeneratedMessageV3
)

data class SurfaceScenarioData(
    val scenarioData: Map<String, Any>
)

data class MementoChangeUserObjectsDirective(
    val userConfigs: Map<EConfigKey, UserConfig>? = null,
    val scenarioData: Any? = null,
    val surfaceScenarioData: SurfaceScenarioData? = null,
): ServerDirective
