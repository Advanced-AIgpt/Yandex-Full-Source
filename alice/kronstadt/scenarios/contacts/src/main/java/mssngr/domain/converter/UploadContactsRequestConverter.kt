package ru.yandex.alice.kronstadt.scenarios.contacts.mssngr.domain.converter

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.scenarios.contacts.mssngr.domain.MssngrApiUploadContactsRequest
import ru.yandex.alice.protos.data.Contacts.TUpdateContactsRequest

@Component
open class UploadContactsRequestConverter {

    private val telegramType = "org.telegram.messenger"

    fun convert(request: TUpdateContactsRequest,
        uuid: String,
        oldSyncKey: Long,
        newSyncKey: Long)
    : MssngrApiUploadContactsRequest {
        return MssngrApiUploadContactsRequest(
            params = MssngrApiUploadContactsRequest.Params(
                uuid = uuid,
                oldSyncKey = oldSyncKey,
                newSyncKey = newSyncKey,
                updatedContacts = request.updatedContactsList.map { convertContactInfo(it) },
                createdContacts = request.createdContactsList.map { convertContactInfo(it) },
                removedContacts = request.removedContactsList.map { convertLookupInfo(it) }
            )
        )
    }

    private fun convertContactInfo(contact: TUpdateContactsRequest.TContactInfo): MssngrApiUploadContactsRequest.ContactInfo {
        if (contact.hasTelegramContactInfo()) {
            return convertTelegramContactInfo(contact.telegramContactInfo)
        }
        throw IllegalStateException("Unsupported contactInfo type")
    }

    private fun convertTelegramContactInfo(contact: TUpdateContactsRequest.TTelegramContactInfo): MssngrApiUploadContactsRequest.ContactInfo {
        return MssngrApiUploadContactsRequest.ContactInfo(
            lookupKey = telegramLookupKey(contact),
            contactId = try {
                contact.contactId.toLong()
            } catch (ex: Exception) {
                throw IllegalStateException("Failed parse contactId from: ${contact.contactId}. ContactId must be unit64")
            },
            accountType = "${telegramType}_${contact.userId}",
            accountName = contact.contactId,
            displayName = "${contact.firstName} ${contact.secondName}",
            firstName = contact.firstName,
            secondName = contact.secondName
        )
    }

    private fun convertLookupInfo(contact: TUpdateContactsRequest.TContactInfo): MssngrApiUploadContactsRequest.LookupInfo {
        if (contact.hasTelegramContactInfo()) {
            return convertTelegramContactInfoToLookupInfo(contact.telegramContactInfo)
        }
        throw IllegalStateException("Unsupported contactInfo type")
    }

    private fun convertTelegramContactInfoToLookupInfo(contact: TUpdateContactsRequest.TTelegramContactInfo): MssngrApiUploadContactsRequest.LookupInfo {
        return MssngrApiUploadContactsRequest.LookupInfo(
            lookupKey = telegramLookupKey(contact)
        )
    }

    private fun telegramLookupKey(contact: TUpdateContactsRequest.TTelegramContactInfo) =
        "${telegramType}_${contact.userId}_${contact.contactId}"
}
