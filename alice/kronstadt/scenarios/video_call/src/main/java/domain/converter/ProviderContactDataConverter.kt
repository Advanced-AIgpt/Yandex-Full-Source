package ru.yandex.alice.kronstadt.scenarios.video_call.domain.converter

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.convert.request.FromProtoConverter
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoContext
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoConverter
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.Provider
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.ProviderContactData
import ru.yandex.alice.protos.data.scenario.video_call.VideoCall.TProviderContactData


@Component
object ProviderContactDataConverter:
    ToProtoConverter<ProviderContactData, TProviderContactData>,
    FromProtoConverter<TProviderContactData, ProviderContactData>
{
    override fun convert(
        src: ProviderContactData,
        ctx: ToProtoContext
    ): TProviderContactData {
        val builder = TProviderContactData.newBuilder()
        when(src.provider) {
             Provider.Telegram -> {
                builder.setTelegramContactData(
                    TProviderContactData.TTelegramContactData.newBuilder()
                        .setUserId(src.userId)
                        .setDisplayName(src.displayName)
                )
            }
        }
        return builder.build()
    }

    override fun convert(src: TProviderContactData) = if (src.hasTelegramContactData()) {
        ProviderContactData(
            provider = Provider.Telegram,
            userId = src.telegramContactData.userId,
            displayName = src.telegramContactData.displayName)
    } else error("Unsupported contact type")
}
