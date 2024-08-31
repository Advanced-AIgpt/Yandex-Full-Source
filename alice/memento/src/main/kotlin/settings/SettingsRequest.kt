package ru.yandex.alice.memento.settings

import ru.yandex.alice.memento.proto.MementoApiProto.EConfigKey
import ru.yandex.alice.memento.proto.MementoApiProto.EDeviceConfigKey

data class SettingsRequest(
    val uid: String,
    val anonymous: Boolean,
    val userSettingsKeys: Set<EConfigKey>,
    val deviceSettingsKeys: Map<String, Set<EDeviceConfigKey>>,
    val scenarios: Set<String>,
    val surfaceScenarios: Map<String, Set<String>>,
)
