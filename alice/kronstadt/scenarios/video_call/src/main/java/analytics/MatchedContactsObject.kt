package ru.yandex.alice.kronstadt.scenarios.video_call.analytics

import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoObject
import ru.yandex.alice.kronstadt.core.domain.Contact
import ru.yandex.alice.megamind.protos.scenarios.AnalyticsInfo.TAnalyticsInfo.TObject
import ru.yandex.alice.protos.data.Contacts
import javax.annotation.Nonnull

data class MatchedContactsObject(
    val contacts: List<Contact>
) : AnalyticsInfoObject(
    "call_contacts",
    "matched contacts",
    "Найденные контакты"
) {
    @Nonnull
    public override fun fillProtoField(@Nonnull protoBuilder: TObject.Builder):
        TObject.Builder = protoBuilder.setMatchedContacts(
            Contacts.TContactsList.newBuilder()
            .addAllContacts(
                contacts.map {
                    Contacts.TContactsList.TContact.newBuilder().apply {
                        contactId = it.contactId
                        lookupKey = it.getFilledLookupKey()
                        displayName = it.displayName ?: ""
                    }.build()
                },
        ).build()
    )

}
