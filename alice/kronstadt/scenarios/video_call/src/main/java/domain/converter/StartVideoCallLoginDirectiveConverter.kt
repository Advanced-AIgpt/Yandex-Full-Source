package ru.yandex.alice.kronstadt.scenarios.video_call.domain.converter

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.convert.response.DirectiveConverterBase
import ru.yandex.alice.kronstadt.core.convert.response.SemanticFrameRequestDataConverter
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoContext
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.StartVideoCallLoginDirective
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.StartVideoCallLoginDirective.TelegramStartLoginData
import ru.yandex.alice.megamind.protos.scenarios.directive.TDirective
import ru.yandex.alice.protos.endpoint.CapabilityProto.TVideoCallCapability
import ru.yandex.alice.protos.endpoint.CapabilityProto.TVideoCallCapability.TStartVideoCallLoginDirective.TTelegramStartLoginData

@Component
open class StartVideoCallLoginDirectiveConverter(
    private val semanticFrameRequestDataConverter: SemanticFrameRequestDataConverter
) : DirectiveConverterBase<StartVideoCallLoginDirective> {

    override fun convert(src: StartVideoCallLoginDirective, ctx: ToProtoContext): TDirective {
        val directive = TVideoCallCapability.TStartVideoCallLoginDirective.newBuilder()
            .setName("video_call__start_login")
            .setTelegramStartLoginData(
                when (src.providerData) {
                    is TelegramStartLoginData -> convertTelegramStartLoginData(src.providerData, ctx)
                }
            )
        return TDirective.newBuilder()
            .setStartVideoCallLoginDirective(directive)
            .build()
    }

    private fun convertTelegramStartLoginData(
        src: TelegramStartLoginData,
        ctx: ToProtoContext
    ): TTelegramStartLoginData {
        return TTelegramStartLoginData.newBuilder().apply {
            id = src.id
            if (src.onFailCallback != null) {
                onFailCallback = semanticFrameRequestDataConverter.convertToStruct(src.onFailCallback, ctx)
            }
        }.build()
    }

    override val directiveType: Class<StartVideoCallLoginDirective>
        get() = StartVideoCallLoginDirective::class.java
}
