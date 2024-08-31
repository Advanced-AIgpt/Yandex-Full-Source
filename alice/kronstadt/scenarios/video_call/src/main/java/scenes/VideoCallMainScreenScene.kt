package ru.yandex.alice.kronstadt.scenarios.video_call.scenes

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RelevantResponse
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.VideoCallCapability
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoContext
import ru.yandex.alice.kronstadt.core.domain.Contact
import ru.yandex.alice.kronstadt.core.domain.ContactsList
import ru.yandex.alice.kronstadt.core.layout.Layout
import ru.yandex.alice.kronstadt.core.scenario.AbstractNoargScene
import ru.yandex.alice.kronstadt.scenarios.video_call.CENTAUR_COLLECT_MAIN_SCREEN
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.MainScreenScenarioData
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.MainScreenScenarioData.Companion.LOGGED_OUT_MAIN_SCREEN_DATA
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.MainScreenScenarioData.TelegramCardData
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.WidgetScenarioData
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.WidgetScenarioData.Companion.LOGGED_OUT_WIDGET_DATA
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.converter.VideoCallScenarioDataConverter
import ru.yandex.alice.kronstadt.scenarios.video_call.utils.ContactUtils.matchContactsByLookupKeys
import ru.yandex.alice.kronstadt.scenarios.video_call.utils.VideoCallCapabilityExtension.checkTelegramContactsUpload
import ru.yandex.alice.kronstadt.scenarios.video_call.utils.VideoCallCapabilityExtension.checkTelegramLogin
import ru.yandex.alice.kronstadt.scenarios.video_call.utils.VideoCallCapabilityExtension.getTelegramAccountType
import ru.yandex.alice.kronstadt.scenarios.video_call.utils.VideoCallCapabilityExtension.getTelegramUserId
import ru.yandex.alice.megamind.protos.scenarios.RequestProto
import ru.yandex.kronstadt.alice.scenarios.video_call.proto.TVideoCallScenarioData

const val WIDGET_WITH_FAVORITES_EXP = "video_call_widget_with_favorites"
const val SCENARIO_WIDGET_MECHANICS_EXP = "scenario_widget_mechanics"

@Component
class VideoCallMainScreenScene(
   private val videoCallScenarioDataConverter: VideoCallScenarioDataConverter,
) : AbstractNoargScene<Any>(
    name = "video_call_main_screen"
) {
    override fun render(request: MegaMindRequest<Any>): RelevantResponse<Any> {
        val mainScreenScenarioData = if (!request.videoCallCapability!!.checkTelegramLogin()) {
            if (request.hasExperiment(SCENARIO_WIDGET_MECHANICS_EXP)) LOGGED_OUT_WIDGET_DATA
            else LOGGED_OUT_MAIN_SCREEN_DATA
        } else {
            require(request.contactsList != null) { "Contacts datasource not provided" }
            val accountType = request.videoCallCapability?.getTelegramAccountType()
                ?: error("Failed to get telegram login from capability state")

            val favoritesContacts = if (request.videoCallCapability!!.checkTelegramContactsUpload()) {
                if (request.hasExperiment(WIDGET_WITH_FAVORITES_EXP))
                    getFavorites(request.mementoData, request.contactsList!!, accountType)
                else getFirstContacts(request.contactsList!!)
            } else listOf()
            if (request.hasExperiment(SCENARIO_WIDGET_MECHANICS_EXP))
                getWidgetScenarioData(request.videoCallCapability!!, favoritesContacts)
            else
                getMainScreenScenarioData(request.videoCallCapability!!, favoritesContacts)
        }
        return RunOnlyResponse(
            layout = Layout.silence(),
            state = null,
            analyticsInfo = AnalyticsInfo(intent = CENTAUR_COLLECT_MAIN_SCREEN),
            isExpectsRequest = false,
            scenarioData = videoCallScenarioDataConverter.convert(mainScreenScenarioData, ToProtoContext())
        )
    }

    private fun getWidgetScenarioData(
        capability: VideoCallCapability,
        favoritesContacts: List<Contact>,
    ) = WidgetScenarioData(
        WidgetScenarioData.TelegramCardData(
            loggedIn = true,
            contactsUploaded = capability.checkTelegramContactsUpload(),
            userId = capability.getTelegramUserId()!!,
            favoriteContactData = favoritesContacts.map {
                WidgetScenarioData.FavoriteContactData(
                    userId = it.contactId.toString(),
                    displayName = it.displayName!!,
                    lookupKey = it.getFilledLookupKey()
                )
            }
        )
    )

    private fun getMainScreenScenarioData(
        capability: VideoCallCapability,
        favoritesContacts: List<Contact>,
    ) = MainScreenScenarioData(
        TelegramCardData(
            loggedIn = true,
            contactsUploaded = capability.checkTelegramContactsUpload(),
            userId = capability.getTelegramUserId()!!,
            favoriteContactData = favoritesContacts.map {
                    MainScreenScenarioData.FavoriteContactData(
                        userId = it.contactId.toString(),
                        displayName = it.displayName!!,
                        lookupKey = it.getFilledLookupKey()
                    )
                }
        )
    )

    private fun getFavorites(
        mementoData: RequestProto.TMementoData,
        contactsList: ContactsList,
        accountType: String,
    ): List<Contact> =
        getMementoScenarioData(mementoData)?.let { it ->
            matchContactsByLookupKeys(contactsList, accountType, it.favoritesList.map { it.lookupKey })
        } ?: listOf()

    private fun getMementoScenarioData(mementoData: RequestProto.TMementoData): TVideoCallScenarioData? =
        if (mementoData.scenarioData != null && mementoData.scenarioData.`is`(TVideoCallScenarioData::class.java)) {
            mementoData.scenarioData.unpack(TVideoCallScenarioData::class.java)
        } else null

    // get first four contacts while we haven't favorite contacts
    private fun getFirstContacts(contactsList: ContactsList) = contactsList.contacts.take(6)
}

