package ru.yandex.alice.kronstadt.scenarios.video_call.domain.converter

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoContext
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoConverter
import ru.yandex.alice.kronstadt.core.domain.scenariodata.ScenarioData
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.ContactChoosingScenarioData
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.CurrentTelegramCallScenarioData
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.IncomingTelegramCallScenarioData
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.MainScreenScenarioData
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.OutgoingTelegramCallScenarioData
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.WidgetScenarioData
import ru.yandex.alice.protos.data.scenario.Data
import ru.yandex.alice.protos.data.scenario.centaur.MainScreen
import ru.yandex.alice.protos.data.scenario.video_call.VideoCall

@Component
open class VideoCallScenarioDataConverter (
    private val mainScreenScenarioDataConverter: MainScreenScenarioDataConverter,
    private val widgetScenarioDataConverter: WidgetScenarioDataConverter,
    private val contactChoosingScenarioDataConverter: ContactChoosingScenarioDataConverter,
    private val providerContactDataConverter: ProviderContactDataConverter,
) : ToProtoConverter<ScenarioData, Data.TScenarioData> {
    override fun convert(src: ScenarioData, ctx: ToProtoContext): Data.TScenarioData {
        when (src) {
            is MainScreenScenarioData -> return convertMainScreenScenarioData(src, ctx)
            is WidgetScenarioData -> return convertWidgetScenarioData(src, ctx)
            is ContactChoosingScenarioData -> return convertContactChoosingScenarioData(src, ctx)
            is IncomingTelegramCallScenarioData -> return convertIncomingTelegramCallScenarioData(src, ctx)
            is OutgoingTelegramCallScenarioData -> return convertOutgoingTelegramCallScenarioData(src, ctx)
            is CurrentTelegramCallScenarioData -> return convertCurrentTelegramCallScenarioData(src, ctx)
        }
        throw RuntimeException("Can't find converter for scenario_data of type: " + src.javaClass.name)
    }

    private fun convertMainScreenScenarioData(
        src: MainScreenScenarioData,
        ctx: ToProtoContext
    ) = Data.TScenarioData.newBuilder()
            .setVideoCallMainScreenData(mainScreenScenarioDataConverter.convert(src, ctx))
            .build()

    private fun convertWidgetScenarioData(
        src: WidgetScenarioData,
        ctx: ToProtoContext
    ) = Data.TScenarioData.newBuilder()
        .setCentaurScenarioWidgetData(
            MainScreen.TCentaurScenarioWidgetData.newBuilder()
                .setWidgetType("video_call")
                .addWidgetCards(widgetScenarioDataConverter.convert(src, ctx))
        )
        .build()

    private fun convertContactChoosingScenarioData(
        src: ContactChoosingScenarioData,
        ctx: ToProtoContext
    ) = Data.TScenarioData.newBuilder()
            .setVideoCallContactChoosingData(contactChoosingScenarioDataConverter.convert(src, ctx))
            .build()

    private fun convertIncomingTelegramCallScenarioData(
        src: IncomingTelegramCallScenarioData,
        ctx: ToProtoContext
    ) = Data.TScenarioData.newBuilder()
            .setIncomingTelegramCallData(
                VideoCall.TIncomingTelegramCallData.newBuilder()
                    .setUserId(src.userId)
                    .setCallId(src.callId)
                    .setCaller(providerContactDataConverter.convert(src.caller, ctx))
                    .build()
            )
            .build()

    private fun convertOutgoingTelegramCallScenarioData(
        src: OutgoingTelegramCallScenarioData,
        ctx: ToProtoContext
    ) = Data.TScenarioData.newBuilder()
        .setOutgoingTelegramCallData(
            VideoCall.TOutgoingTelegramCallData.newBuilder()
                .setUserId(src.userId)
                .setRecipient(providerContactDataConverter.convert(src.recipient, ctx))
                .build()
        )
        .build()

    private fun convertCurrentTelegramCallScenarioData(
        src: CurrentTelegramCallScenarioData,
        ctx: ToProtoContext
    ) = Data.TScenarioData.newBuilder()
        .setCurrentTelegramCallData(
            VideoCall.TCurrentTelegramCallData.newBuilder()
                .setUserId(src.userId)
                .setCallId(src.callId)
                .setRecipient(providerContactDataConverter.convert(src.recipient, ctx))
                .build()
        )
        .build()
}
