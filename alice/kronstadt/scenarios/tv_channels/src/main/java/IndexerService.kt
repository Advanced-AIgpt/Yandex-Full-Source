package ru.yandex.alice.kronstadt.scenarios.tv_channels

import org.apache.logging.log4j.LogManager
import org.springframework.stereotype.Component
import org.springframework.web.util.UriComponentsBuilder
import ru.yandex.alice.kronstadt.scenarios.tv_channels.indexer.DeleteMessage
import ru.yandex.alice.kronstadt.scenarios.tv_channels.indexer.ModifyMessage
import ru.yandex.alice.kronstadt.scenarios.tv_channels.indexer.SaasIndexerClient
import ru.yandex.alice.kronstadt.scenarios.tv_channels.indexer.TypedItemType
import ru.yandex.alice.kronstadt.scenarios.tv_channels.search.FoundDocument
import ru.yandex.alice.kronstadt.scenarios.tv_channels.search.SaasSearchClient

const val DEAULT_ALIASES = "канал программа кнопка"

@Component
class IndexerService(
    private val saasSearchClient: SaasSearchClient,
    private val indexerClient: SaasIndexerClient,
) {

    private val logger = LogManager.getLogger()

    fun createIndexMessages(
        quasarDeviceId: String,
        timestamp: Long,
        channels: List<Channel>
    ): List<ModifyMessage> {
        val docs = channels.map {
            DeviceChannelData(
                deviceId = quasarDeviceId,
                title = it.name,
                uri = UriComponentsBuilder.fromUriString(it.uri)
                    .queryParam("device_id", quasarDeviceId)
                    .queryParam("ts", timestamp)
                    .build()
                    .toUriString(),
                number = it.number,
            )
        }
            .map { it.toModifyMessage(timestamp) }
        return docs
    }

    fun cleanOldDocuments(deviceId: String, timestamp: Long, exclude: Set<String> = setOf()) {
        logger.info("Start clearing old channels for device ${deviceId}")
        val query = "s_device_id:${deviceId}"
        val foundDocs = saasSearchClient.search(query)

        logger.info("Found ${foundDocs.size} already existing documents for device ${deviceId}")

        val docsToRemove = foundDocs.filter {
            it.timestamp != null &&
                it.timestamp < timestamp &&
                it.timestamp > 0 &&
                it.url !in exclude
        }

        logger.info("Remove ${docsToRemove.size} old documents for device ${deviceId}")
        if (docsToRemove.isNotEmpty()) {
            logger.info("Deleting ${docsToRemove.size} old documents")
            indexerClient.sendDocuments(docsToRemove.map { it.toDeleteMessage() })
        }
        logger.info("Clearing old channels for device ${deviceId} completed")
    }

    fun indexAllDocuments(
        quasarDeviceId: String,
        timestamp: Long,
        channels: List<Channel>
    ) {
        logger.info("Indexing ${channels.size} channels for device ${quasarDeviceId}")
        val msgs = createIndexMessages(quasarDeviceId, timestamp, channels)

        indexerClient.sendDocuments(msgs)
        logger.info("Channels indexing for device ${quasarDeviceId} completed")
    }
}

private data class DeviceChannelData(
    val deviceId: String,
    val title: String,
    val uri: String,
    val number: Int?,
    val regionIds: List<Int> = listOf(),
    val extra: Any? = null,
)

private fun DeviceChannelData.toModifyMessage(timestamp: Long?): ModifyMessage {
    val msg = ModifyMessage(this.uri)
    msg.addZone("title", this.title)
    msg.addAttr("s_device_id", this.deviceId, TypedItemType.SEARCH_ATTR_LITERAL, TypedItemType.PROPERTY)

    this.regionIds.forEach {
        msg.addAttr("i_region_id", it, TypedItemType.INT_ATTR, TypedItemType.PROPERTY)
    }

    if (this.number != null) {
        msg.addZone("z_number", "${this.number} ${DEAULT_ALIASES}")
        msg.addAttr("number", number.toString(), TypedItemType.PROPERTY)
    }

    if (timestamp != null) {
        msg.addAttr("i_ts", timestamp, TypedItemType.INT_ATTR, TypedItemType.PROPERTY)
    }

    if (extra != null) {
        msg.addZone("z_extra", extra)
    }

    return msg
}

private fun FoundDocument.toDeleteMessage(): DeleteMessage = DeleteMessage(this.url)
