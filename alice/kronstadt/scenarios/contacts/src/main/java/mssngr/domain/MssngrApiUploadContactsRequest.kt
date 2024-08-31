package ru.yandex.alice.kronstadt.scenarios.contacts.mssngr.domain

import com.fasterxml.jackson.annotation.JsonProperty

data class MssngrApiUploadContactsRequest(
    val params: Params,
    val method: String = "upload_contacts_alice_v2",
) {
    data class Params(
        @JsonProperty("uuid") val uuid: String,
        @JsonProperty("OldSyncKey") val oldSyncKey: Long,
        @JsonProperty("NewSyncKey") val newSyncKey: Long,
        @JsonProperty("UpdatedContacts") val updatedContacts: List<ContactInfo>? = null,
        @JsonProperty("CreatedContacts") val createdContacts: List<ContactInfo>? = null,
        @JsonProperty("RemovedContacts") val removedContacts: List<LookupInfo>? = null,
        @JsonProperty("UpdatedPhones") val updatedPhones: List<PhoneInfo>? = null,
        @JsonProperty("RemovedPhones") val removedPhones: List<LookupInfo>? = null,
    )

    data class PhoneInfo(
        @JsonProperty("LookupKey") val lookupKey: String,
        @JsonProperty("Id") val id: Long,
        @JsonProperty("AccountType") val accountType: String,
        @JsonProperty("Phone") val phone: String,
        @JsonProperty("Type") val type: String,
        @JsonProperty("IdString") val idString: String? = null,
    )

    data class LookupInfo(
        @JsonProperty("LookupKey") val lookupKey: String,
        @JsonProperty("Id") val id: Long? = null,
    )

    data class ContactInfo(
        @JsonProperty("LookupKey") val lookupKey: String,
        @JsonProperty("ContactId") val contactId: Long,
        @JsonProperty("AccountType") val accountType: String,
        @JsonProperty("Id") val id: Long? = null,
        @JsonProperty("AccountName") val accountName: String? = null,
        @JsonProperty("DisplayName") val displayName: String? = null,
        @JsonProperty("FirstName") val firstName: String? = null,
        @JsonProperty("MiddleName") val middleName: String? = null,
        @JsonProperty("SecondName") val secondName: String? = null,
        @JsonProperty("TimesContacted") val timesContacted: Int? = null,
        @JsonProperty("LastTimeContacted") val lastTimeContacted: Long? = null,
        @JsonProperty("Phones") val phones: List<PhoneInfo>? = null,
    )
}
