package ru.yandex.alice.kronstadt.scenarios.tv_channels.indexer

interface SaasIndexerClient {
    fun sendDocuments(msgs: List<SaasMessage>)
}
