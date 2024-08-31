package ru.yandex.alice.kronstadt.scenarios.video_call.scenes

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RelevantResponse
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.directive.server.MementoChangeUserObjectsDirective
import ru.yandex.alice.kronstadt.core.directive.server.ServerDirective
import ru.yandex.alice.kronstadt.core.layout.Layout
import ru.yandex.alice.kronstadt.core.scenario.AbstractNoargScene
import ru.yandex.alice.kronstadt.scenarios.video_call.SET_FAVORITES
import ru.yandex.alice.kronstadt.scenarios.video_call.utils.ContactUtils.matchContactsByTelegramIds
import ru.yandex.alice.kronstadt.scenarios.video_call.utils.VideoCallCapabilityExtension.getTelegramAccountType
import ru.yandex.kronstadt.alice.scenarios.video_call.proto.TFavoriteContact
import ru.yandex.kronstadt.alice.scenarios.video_call.proto.TVideoCallScenarioData

@Component
object SetFavoritesScene
    : AbstractNoargScene<Any>(
    name = "set_favorites_scene"
) {
    override fun render(request: MegaMindRequest<Any>): RelevantResponse<Any> {
        require(request.contactsList != null) {"Contacts datasource not provided"}
        val accountType = request.videoCallCapability?.getTelegramAccountType()
            ?: error("Failed to get telegram login from capability state")

        val contactsForFavorites = request.getSemanticFrame(SET_FAVORITES)!!.typedSemanticFrame
            ?.videoCallSetFavoritesSemanticFrame?.favorites?.contactList?.contactDataList
            ?.map { it.telegramContactData.userId }
            ?.let {  matchContactsByTelegramIds(request.contactsList!!, accountType, it) }
            ?: error("Not provided contacts in TSF for setting favorites")

        val mementoDirective = createMementoChangeUserObjectDirective(
            contactsForFavorites.map { it.getFilledLookupKey() }
        )

        return RunOnlyResponse(
            layout = Layout.silence(),
            state = null,
            analyticsInfo = AnalyticsInfo(intent = SET_FAVORITES),
            isExpectsRequest = false,
            serverDirectives = listOf(mementoDirective),
        )
    }

    private fun createMementoChangeUserObjectDirective(favoritesKeys: List<String>): ServerDirective {
        val newMementoDataBuilder = TVideoCallScenarioData.newBuilder()
            .addAllFavorites(
                favoritesKeys.map {
                    TFavoriteContact.newBuilder()
                        .setLookupKey(it)
                        .build()
                }
            )
        return MementoChangeUserObjectsDirective(
            scenarioData = com.google.protobuf.Any.pack(newMementoDataBuilder.build())
        )
    }
}
