package ru.yandex.alice.kronstadt.scenarios.video_call.domain.converter

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoContext
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoConverter
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.ContactChoosingScenarioData
import ru.yandex.alice.protos.data.scenario.video_call.VideoCall

@Component
class ContactChoosingScenarioDataConverter(
    private val providerContactDataConverter: ProviderContactDataConverter
):
    ToProtoConverter<ContactChoosingScenarioData, VideoCall.TVideoCallContactChoosingData>
{
    override fun convert(
        src: ContactChoosingScenarioData,
        ctx: ToProtoContext
    ): VideoCall.TVideoCallContactChoosingData {
        val builder = VideoCall.TVideoCallContactChoosingData.newBuilder()
        builder.addAllContactData(src.contactData.map { providerContactDataConverter.convert(it, ctx) })
        return builder.build()
    }
}
