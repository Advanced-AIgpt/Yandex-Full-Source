package ru.yandex.alice.kronstadt.scenarios.video_call.domain.converter

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.convert.response.DirectiveConverterBase
import ru.yandex.alice.kronstadt.core.convert.response.SemanticFrameRequestDataConverter
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoContext
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.StartVideoCallDirective
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.StartVideoCallDirective.TelegramStartVideoCallData
import ru.yandex.alice.megamind.protos.scenarios.directive.TDirective
import ru.yandex.alice.protos.endpoint.CapabilityProto.TVideoCallCapability
import ru.yandex.alice.protos.endpoint.CapabilityProto.TVideoCallCapability.TStartVideoCallDirective.TTelegramStartVideoCallData

@Component
open class StartVideoCallDirectiveConverter(
    private val semanticFrameRequestDataConverter: SemanticFrameRequestDataConverter
) : DirectiveConverterBase<StartVideoCallDirective> {

    override fun convert(src: StartVideoCallDirective, ctx: ToProtoContext): TDirective {
        val directive = TVideoCallCapability.TStartVideoCallDirective.newBuilder()
            .setName("video_call__start_video_call")
            .setTelegramStartVideoCallData(
                when (src.providerData) {
                    is TelegramStartVideoCallData -> convertTelegramStartVideoCallData(src.providerData, ctx)
                }
            )
        return TDirective.newBuilder()
            .setStartVideoCallDirective(directive)
            .build()
    }

    private fun convertTelegramStartVideoCallData(
        src: TelegramStartVideoCallData,
        ctx: ToProtoContext
    ): TTelegramStartVideoCallData {
        return TTelegramStartVideoCallData.newBuilder().apply {
            id = src.id
            userId = src.userId
            recipientUserId = src.recipientUserId
            videoEnabled = src.videoEnabled
            if (src.onAcceptedCallback != null) {
                onAcceptedCallback = semanticFrameRequestDataConverter.convertToStruct(src.onAcceptedCallback, ctx)
            }
            if (src.onDiscardedCallback != null) {
                onDiscardedCallback = semanticFrameRequestDataConverter.convertToStruct(src.onDiscardedCallback, ctx)
            }
            if (src.onFailCallback != null) {
                onFailCallback = semanticFrameRequestDataConverter.convertToStruct(src.onFailCallback, ctx)
            }
        }.build()
    }

    override val directiveType: Class<StartVideoCallDirective>
        get() = StartVideoCallDirective::class.java
}
