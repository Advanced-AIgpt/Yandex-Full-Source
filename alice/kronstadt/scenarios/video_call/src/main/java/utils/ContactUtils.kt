package ru.yandex.alice.kronstadt.scenarios.video_call.utils

import org.apache.logging.log4j.LogManager
import ru.yandex.alice.kronstadt.core.domain.Contact
import ru.yandex.alice.kronstadt.core.domain.ContactsList
import ru.yandex.alice.kronstadt.core.domain.TELEGRAM_CONTACT_TYPE_PREFIX

object ContactUtils {
    private val logger = LogManager.getLogger(ContactUtils::class.java)

    fun matchContactsByTelegramIds(
        contactsList: ContactsList,
        accountType: String,
        telegramIds: List<String>,
    ): List<Contact> =
        telegramIds.mapNotNull {
            contactsList.matchContactByTelegramId(accountType, it)
                ?: run { logger.warn("Can't find matching contact for telegram id: $it")
                    return@mapNotNull null }
        }

    fun matchContactsByLookupKeys(
        contactsList: ContactsList,
        accountType: String,
        lookupKeysOrIndexes: List<String>,
    ): List<Contact> =
        lookupKeysOrIndexes.mapNotNull {
            contactsList.matchContactByLookupKey(accountType, it)
                ?: run { logger.warn("Can't find matching contact for lookupKey: $it")
                    return@mapNotNull null }
        }
}

fun ContactsList.matchContactByUserIdAndContactId(userId: String, contactId: String): Contact? =
    contacts.firstOrNull {
        it.accountType == "${TELEGRAM_CONTACT_TYPE_PREFIX}_$userId" &&
            it.contactId.toString() == contactId
    }

fun ContactsList.matchContactByTelegramId(accountType: String, telegramId: String): Contact? =
    contacts.firstOrNull {
        it.accountType == accountType &&
        it.contactId.toString() == telegramId
    }

fun ContactsList.matchContactByLookupKey(accountType: String, lookupKey: String): Contact? =
    contacts.firstOrNull {
        it.accountType == accountType && it.getFilledLookupKey() == lookupKey
    }

fun ContactsList.filterByLookupKeysOrIndexes(accountType: String, lookupKeysOrIndexes: List<String>): List<Contact> =
    contacts.filter {
        it.accountType == accountType &&
            (it.getFilledLookupKey() in lookupKeysOrIndexes || it.lookupKeyIndex.toString() in lookupKeysOrIndexes)
    }