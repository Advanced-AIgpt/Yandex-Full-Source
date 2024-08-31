package ru.yandex.alice.kronstadt.scenarios.video_call.analytics

import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoAction
import ru.yandex.alice.kronstadt.core.domain.Contact
import ru.yandex.alice.megamind.protos.scenarios.AnalyticsInfo
import ru.yandex.alice.protos.data.Contacts

class VideoCallToAction (
    private val callee: Contact,
) : AnalyticsInfoAction(
    "call.video_call_to",
    "call to telegram",
    "Совершается звонок в телеграм"
) {

    override fun fillProtoField(protoBuilder: AnalyticsInfo.TAnalyticsInfo.TAction.Builder):
        AnalyticsInfo.TAnalyticsInfo.TAction.Builder = protoBuilder.setCallee(
            Contacts.TContactsList.TContact.newBuilder()
                .setContactId(callee.contactId)
                .setLookupKey(callee.getFilledLookupKey())
                .setDisplayName(callee.displayName ?: "")
        )
}