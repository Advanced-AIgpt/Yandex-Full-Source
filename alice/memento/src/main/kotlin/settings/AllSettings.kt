package ru.yandex.alice.memento.settings

import ru.yandex.alice.memento.proto.MementoApiProto.TSurfaceConfig
import ru.yandex.alice.memento.proto.MementoApiProto.TUserConfigs
import com.google.protobuf.Any as ProtoAny

data class AllSettings(
    val userSettings: TUserConfigs,
    val surfacesSettings: Map<String, TSurfaceConfig>,
    val scenariosData: Map<String, ProtoAny>,
    val surfaceScenarioData: Map<String, Map<String, ProtoAny>>,
)
