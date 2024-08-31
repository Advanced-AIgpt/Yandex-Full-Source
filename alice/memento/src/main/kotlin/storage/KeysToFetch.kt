package ru.yandex.alice.memento.storage

data class KeysToFetch(
    val userKeys: Set<String>,
    val devicesKeys: Map<String, Set<String>>,
    val scenarios: Set<String>,
    val surfaceScenarios: Map<String, Set<String>>,
)
