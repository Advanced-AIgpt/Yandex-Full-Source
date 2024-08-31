package ru.yandex.alice.kronstadt.core.domain

import com.google.protobuf.ByteString

const val TELEGRAM_CONTACT_TYPE_PREFIX = "org.telegram.messenger"

data class ContactsList(
    val isKnownUuid: Boolean,
    val deleted: Boolean,
    val contacts: List<Contact> = listOf(),
    val phones: List<Phone> = listOf(),
    val lookupKeyMapSerialized: ByteString? = null,
    val truncated: Boolean = false,
    private val lookupKeyMapLazy: Lazy<Map<Int, String>?> = lazyOf(null),
) {
    val lookupKeyMap: Map<Int, String>? by lookupKeyMapLazy

    data class Phone(
        val id: Int,
        val accountType: String,
        val lookupKey: String,
        val phone: String,
        val idString: String,
        val type: String?,
    )

    override fun hashCode(): Int {
        var result = isKnownUuid.hashCode()
        result = 31 * result + deleted.hashCode()
        result = 31 * result + contacts.hashCode()
        result = 31 * result + phones.hashCode()
        result = 31 * result + truncated.hashCode()
        return result
    }

    // Not using for now: https://st.yandex-team.ru/DIALOG-8606
    fun getLookupKeyFor(contact: Contact): String =
        if (!contact.lookupKey.isNullOrEmpty()) contact.lookupKey else lookupKeyMap?.get(contact.lookupKeyIndex)
            ?: error("Failed getting lookupKey from field or from lookupKeyMap by lookupKeyIndex for contact: $contact")
}

data class Contact(
    val id: Int,
    val accountType: String,
    val contactId: Long,
    val lookupKeyIndex: Int?,
    val lookupKey: String?,
    val accountName: String?,
    val displayName: String?,
    val firstName: String?,
    val middleName: String?,
    val secondName: String?,
    val lastTimeContacted: Long = 0,
    val timesContacted: Int = 0,
) {
    fun getFilledLookupKey(): String = if (!lookupKey.isNullOrEmpty()) lookupKey else "${accountType}_${contactId}"
}
