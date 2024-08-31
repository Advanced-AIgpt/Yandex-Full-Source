package ru.yandex.alice.kronstadt.scenarios.tv_channels.indexer

import org.apache.logging.log4j.LogManager

internal class NoopSaasIndexerClient : SaasIndexerClient {
    private val logger = LogManager.getLogger()
    override fun sendDocuments(msgs: List<SaasMessage>) {
        logger.info("indexing documents: $msgs")
    }
}
