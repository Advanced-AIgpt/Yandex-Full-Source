package ru.yandex.alice.memento.storage

import com.google.protobuf.Any as ProtoAny

data class StoredData(
    val userSettings: Map<String, ProtoAny>,
    val deviceSettings: Map<String, Map<String, ProtoAny>>,
    val scenariosData: Map<String, ProtoAny>,
    val surfaceScenariosData: Map<String, Map<String, ProtoAny>>,
)
