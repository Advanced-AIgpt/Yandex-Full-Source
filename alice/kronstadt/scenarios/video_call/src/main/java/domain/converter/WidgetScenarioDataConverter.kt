package ru.yandex.alice.kronstadt.scenarios.video_call.domain.converter

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoContext
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoConverter
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.WidgetScenarioData
import ru.yandex.alice.protos.data.scenario.centaur.MainScreen.TCentaurWidgetCardData
import ru.yandex.alice.protos.data.scenario.centaur.MainScreen.TCentaurWidgetCardData.TVideoCallCardData

@Component
object WidgetScenarioDataConverter: ToProtoConverter<WidgetScenarioData, TCentaurWidgetCardData> {
    override fun convert(src: WidgetScenarioData, ctx: ToProtoContext): TCentaurWidgetCardData {
        when(val providerData = src.providerData) {
            is WidgetScenarioData.TelegramCardData -> {
                val builder = TVideoCallCardData.newBuilder()

                if (!providerData.loggedIn) {
                    builder.loggedOutCardData = TVideoCallCardData.TLoggedOutCardData.getDefaultInstance()
                } else {
                    builder.loggedInCardData = TVideoCallCardData.TLoggedInCardData.newBuilder()
                        .setTelegramCardData(
                            TVideoCallCardData.TLoggedInCardData.TTelegramCardData.newBuilder()
                                .setContactsUploaded(providerData.contactsUploaded)
                                .setUserId(providerData.userId)
                                .addAllFavoriteContactData(
                                    providerData.favoriteContactData.map {
                                        TVideoCallCardData.TLoggedInCardData.TFavoriteContactData.newBuilder()
                                            .setUserId(it.userId)
                                            .setDisplayName(it.displayName)
                                            .setLookupKey(it.lookupKey)
                                            .build()
                                    }
                                )
                        ).build()
                }
                return TCentaurWidgetCardData.newBuilder()
                    .setVideoCallCardData(builder)
                    .build()
            }
        }
    }
}
