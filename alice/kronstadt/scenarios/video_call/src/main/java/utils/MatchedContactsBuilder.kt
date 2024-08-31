package ru.yandex.alice.kronstadt.scenarios.video_call.utils

import com.fasterxml.jackson.annotation.JsonProperty
import com.fasterxml.jackson.databind.ObjectMapper
import com.fasterxml.jackson.module.kotlin.readValue
import org.apache.logging.log4j.LogManager
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.domain.Contact
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrameSlot
import ru.yandex.alice.kronstadt.scenarios.video_call.IGNORE_CAPABILITY_FOR_UE2E
import ru.yandex.alice.kronstadt.scenarios.video_call.utils.VideoCallCapabilityExtension.getTelegramAccountType

private const val ITEM_NAME_SLOT_TYPE = "device.address_book.item_name"
private const val VARIANTS_SLOT_TYPE = "variants"

@Component
class MatchedContactsBuilder {
    private val logger = LogManager.getLogger(MatchedContactsBuilder::class.java)
    private val objectMapper = ObjectMapper()

    data class MatchedContactSlotValue constructor(
        @JsonProperty(ITEM_NAME_SLOT_TYPE)
        val item: String
    )

    fun buildMatchedContacts(slot: SemanticFrameSlot, request: MegaMindRequest<Any>): List<Contact> {
        val accountType = if (!request.hasExperiment(IGNORE_CAPABILITY_FOR_UE2E))
            request.videoCallCapability!!.getTelegramAccountType()!!
            else request.contactsList?.contacts?.firstOrNull()?.accountType ?: ""
        if (slot.type == VARIANTS_SLOT_TYPE) {
            return try {
                val matchedContactKeys = objectMapper.readValue<List<MatchedContactSlotValue>>(
                    slot.value).map { it.item }
                logger.debug("Matched contact keys: {}", matchedContactKeys)
                return request.contactsList!!.filterByLookupKeysOrIndexes(accountType, matchedContactKeys)
            } catch (e: Exception) {
                logger.warn("Failed deserialize matched contact from frame slot: {}", slot, e)
                listOf()
            }
        } else if (slot.type == ITEM_NAME_SLOT_TYPE) {
            logger.debug("Matched contact key: {}", slot.value)
            return request.contactsList!!.filterByLookupKeysOrIndexes(accountType, listOf(slot.value))
        }
        logger.warn("Failed unpack matched contact from frame slot: $slot")
        return listOf()
    }
}
