package ru.yandex.alice.kronstadt.scenarios.video_call.domain.converter

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoContext
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoConverter
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.MainScreenScenarioData
import ru.yandex.alice.protos.data.scenario.video_call.VideoCall.TVideoCallMainScreenData
import ru.yandex.alice.protos.data.scenario.video_call.VideoCall.TVideoCallMainScreenData.TTelegramCardData

@Component
object MainScreenScenarioDataConverter: ToProtoConverter<MainScreenScenarioData, TVideoCallMainScreenData> {
    override fun convert(src: MainScreenScenarioData, ctx: ToProtoContext): TVideoCallMainScreenData {
        when(val providerData = src.providerData) {
            is MainScreenScenarioData.TelegramCardData -> {
                val builder = TTelegramCardData.newBuilder()
                    .setLoggedIn(providerData.loggedIn)
                    .setContactsUploaded(providerData.contactsUploaded)

                providerData.userId?.apply { builder.userId = this }

                if (providerData.favoriteContactData.isNotEmpty()) {
                    builder.addAllFavoriteContactData(
                        providerData.favoriteContactData.map {
                            TTelegramCardData.TFavoriteContactData.newBuilder()
                                .setUserId(it.userId)
                                .setDisplayName(it.displayName)
                                .setLookupKey(it.lookupKey)
                                .build()
                        }
                    )
                }
                return TVideoCallMainScreenData.newBuilder()
                    .setTelegramCardData(builder)
                    .build()
            }
        }
    }
}
