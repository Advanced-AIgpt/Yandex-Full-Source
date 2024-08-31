package ru.yandex.alice.kronstadt.scenarios.video_call.domain.converter

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.convert.response.DirectiveConverterBase
import ru.yandex.alice.kronstadt.core.convert.response.SemanticFrameRequestDataConverter
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoContext
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.AcceptVideoCallDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TDirective
import ru.yandex.alice.protos.endpoint.CapabilityProto

@Component
open class AcceptVideoCallDirectiveConverter(
    private val semanticFrameRequestDataConverter: SemanticFrameRequestDataConverter
) : DirectiveConverterBase<AcceptVideoCallDirective> {

    override fun convert(src: AcceptVideoCallDirective, ctx: ToProtoContext): TDirective {
        val directive = CapabilityProto.TVideoCallCapability.TAcceptVideoCallDirective.newBuilder()
            .setName("video_call__accept_incoming_video_call")
            .setTelegramAcceptVideoCallData(
                when (src.providerData) {
                    is AcceptVideoCallDirective.TelegramAcceptVideoCallData ->
                        convertTelegramAcceptVideoCallData(src.providerData, ctx)
                }
            )
        return TDirective.newBuilder()
            .setAcceptVideoCallDirective(directive)
            .build()
    }

    private fun convertTelegramAcceptVideoCallData(
        src: AcceptVideoCallDirective.TelegramAcceptVideoCallData,
        ctx: ToProtoContext
    ): CapabilityProto.TVideoCallCapability.TAcceptVideoCallDirective.TTelegramAcceptVideoCallData {
        return CapabilityProto.TVideoCallCapability.TAcceptVideoCallDirective.TTelegramAcceptVideoCallData.newBuilder().apply {
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

    override val directiveType: Class<AcceptVideoCallDirective>
        get() = AcceptVideoCallDirective::class.java
}
