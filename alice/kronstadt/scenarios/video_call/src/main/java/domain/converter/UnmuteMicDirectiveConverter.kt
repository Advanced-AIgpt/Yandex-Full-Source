package ru.yandex.alice.kronstadt.scenarios.video_call.domain.converter

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.convert.response.DirectiveConverterBase
import ru.yandex.alice.kronstadt.core.convert.response.SemanticFrameRequestDataConverter
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoContext
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.UnmuteMicDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TDirective
import ru.yandex.alice.protos.endpoint.CapabilityProto

@Component
open class UnmuteMicDirectiveConverter(
    private val semanticFrameRequestDataConverter: SemanticFrameRequestDataConverter
) : DirectiveConverterBase<UnmuteMicDirective> {

    override fun convert(src: UnmuteMicDirective, ctx: ToProtoContext): TDirective {
        val directive = CapabilityProto.TVideoCallCapability.TUnmuteMicDirective.newBuilder()
            .setName("video_call__unmute_mic")
            .setTelegramUnmuteMicData(
                when (src.providerData) {
                    is UnmuteMicDirective.TelegramUnmuteMicData ->
                        convertTelegramUnmuteMicData(src.providerData, ctx)
                }
            )
        return TDirective.newBuilder()
            .setVideoCallUnmuteMicDirective(directive)
            .build()
    }

    private fun convertTelegramUnmuteMicData(
        src: UnmuteMicDirective.TelegramUnmuteMicData,
        ctx: ToProtoContext
    ): CapabilityProto.TVideoCallCapability.TUnmuteMicDirective.TTelegramUnmuteMicData {
        return CapabilityProto.TVideoCallCapability.TUnmuteMicDirective.TTelegramUnmuteMicData.newBuilder().apply {
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

    override val directiveType: Class<UnmuteMicDirective>
        get() = UnmuteMicDirective::class.java
}
