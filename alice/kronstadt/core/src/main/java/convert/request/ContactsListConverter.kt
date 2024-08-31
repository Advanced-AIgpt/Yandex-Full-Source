package ru.yandex.alice.kronstadt.core.convert.request

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.domain.Contact
import ru.yandex.alice.kronstadt.core.domain.ContactsList
import ru.yandex.alice.kronstadt.core.utils.ContactUtils.deserializeLookupKeyMapping
import ru.yandex.alice.protos.data.Contacts.TContactsList

@Component
open class ContactsListConverter : FromProtoConverter<TContactsList, ContactsList> {
    override fun convert(src: TContactsList): ContactsList {
        return ContactsList(
            isKnownUuid = src.isKnownUuid,
            deleted = src.deleted,
            contacts = src.contactsList.map {
                Contact(
                    id = it.id,
                    accountType = it.accountType,
                    contactId = it.contactId,
                    lookupKeyIndex = it.lookupKeyIndex,
                    lookupKey = it.lookupKey,
                    accountName = it.accountName,
                    displayName = it.displayName,
                    firstName = it.firstName,
                    middleName = it.middleName,
                    secondName = it.secondName,
                    lastTimeContacted = it.lastTimeContacted,
                    timesContacted = it.timesContacted
                )
            },
            phones = src.phonesList.map {
                ContactsList.Phone (
                    id = it.id,
                    accountType = it.accountType,
                    lookupKey = it.lookupKey,
                    phone = it.phone,
                    idString = it.idString,
                    type = it.type
                )
            },
            lookupKeyMapSerialized = src.lookupKeyMapSerialized,
            truncated = src.truncated,
            lookupKeyMapLazy = lazy { src.lookupKeyMapSerialized
                ?.let { deserializeLookupKeyMapping(src.lookupKeyMapSerialized) } },
        )
    }
}
