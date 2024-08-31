package ru.yandex.alice.kronstadt.scenarios.video_call.domain.converter

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.convert.response.DirectiveConverterBase
import ru.yandex.alice.kronstadt.core.convert.response.SemanticFrameRequestDataConverter
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoContext
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.DiscardVideoCallDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TDirective
import ru.yandex.alice.protos.endpoint.CapabilityProto

@Component
open class DiscardVideoCallDirectiveConverter(
    private val semanticFrameRequestDataConverter: SemanticFrameRequestDataConverter
) : DirectiveConverterBase<DiscardVideoCallDirective> {

    override fun convert(src: DiscardVideoCallDirective, ctx: ToProtoContext): TDirective {
        val directive = CapabilityProto.TVideoCallCapability.TDiscardVideoCallDirective.newBuilder()
            .setName("video_call__discard_incoming_video_call")
            .setTelegramDiscardVideoCallData(
                when (src.providerData) {
                    is DiscardVideoCallDirective.TelegramDiscardVideoCallData ->
                        convertTelegramDiscardVideoCallData(src.providerData, ctx)
                }
            )
        return TDirective.newBuilder()
            .setDiscardVideoCallDirective(directive)
            .build()
    }

    private fun convertTelegramDiscardVideoCallData(
        src: DiscardVideoCallDirective.TelegramDiscardVideoCallData,
        ctx: ToProtoContext
    ): CapabilityProto.TVideoCallCapability.TDiscardVideoCallDirective.TTelegramDiscardVideoCallData {
        return CapabilityProto.TVideoCallCapability.TDiscardVideoCallDirective.TTelegramDiscardVideoCallData.newBuilder().apply {
            callOwnerData = CapabilityProto.TVideoCallCapability.TTelegramVideoCallOwnerData.newBuilder()
                .setCallId(src.callOwnerData.callId)
                .setUserId(src.callOwnerData.userId)
                .build()
            if (src.onSuccessCallback != null) {
                onSuccessCallback = semanticFrameRequestDataConverter.convertToStruct(src.onSuccessCallback, ctx)
            }
            if (src.onFailCallback != null) {
                onFailCallback = semanticFrameRequestDataConverter.convertToStruct(src.onFailCallback, ctx)
            }
        }.build()
    }

    override val directiveType: Class<DiscardVideoCallDirective>
        get() = DiscardVideoCallDirective::class.java
}
