package ru.yandex.alice.memento.settings

import ru.yandex.alice.memento.proto.MementoApiProto.EConfigKey
import ru.yandex.alice.memento.proto.MementoApiProto.EDeviceConfigKey
import com.google.protobuf.Any as ProtoAny

data class Settings(
    val userSettings: Map<EConfigKey, ProtoAny>,
    val deviceSettings: Map<String, Map<EDeviceConfigKey, ProtoAny>>,
    val scenariosData: Map<String, ProtoAny>,
    val surfaceScenarioData: Map<String, Map<String, ProtoAny>>,
)
