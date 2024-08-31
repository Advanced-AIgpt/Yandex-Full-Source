package ru.yandex.alice.kronstadt.scenarios.video_call.domain

import ru.yandex.alice.kronstadt.core.domain.Contact
import ru.yandex.alice.kronstadt.core.domain.TELEGRAM_CONTACT_TYPE_PREFIX

enum class Provider {
    Telegram,
}

data class ProviderContactData(
    val userId: String,
    val provider: Provider = Provider.Telegram,
    val displayName: String = "") {
    constructor(contact: Contact): this(
        provider = if (contact.accountType.startsWith(TELEGRAM_CONTACT_TYPE_PREFIX)) Provider.Telegram
        else error("Unsupported contact type"),
        userId = contact.contactId.toString(),
        displayName = contact.displayName ?: ""
    )
}
