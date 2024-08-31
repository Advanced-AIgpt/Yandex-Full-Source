package ru.yandex.alice.kronstadt.scenarios.tv_channels.indexer

sealed interface SaasMessage {
    val action: SaasAction
    val prefix: Int
}
