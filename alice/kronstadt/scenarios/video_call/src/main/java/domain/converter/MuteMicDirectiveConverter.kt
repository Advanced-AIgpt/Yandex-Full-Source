package ru.yandex.alice.kronstadt.scenarios.video_call.domain.converter

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.convert.response.DirectiveConverterBase
import ru.yandex.alice.kronstadt.core.convert.response.SemanticFrameRequestDataConverter
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoContext
import ru.yandex.alice.kronstadt.scenarios.video_call.domain.MuteMicDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TDirective
import ru.yandex.alice.protos.endpoint.CapabilityProto

@Component
open class MuteMicDirectiveConverter(
    private val semanticFrameRequestDataConverter: SemanticFrameRequestDataConverter
) : DirectiveConverterBase<MuteMicDirective> {

    override fun convert(src: MuteMicDirective, ctx: ToProtoContext): TDirective {
        val directive = CapabilityProto.TVideoCallCapability.TMuteMicDirective.newBuilder()
            .setName("video_call__mute_mic")
            .setTelegramMuteMicData(
                when (src.providerData) {
                    is MuteMicDirective.TelegramMuteMicData ->
                        convertTelegramMuteMicData(src.providerData, ctx)
                }
            )
        return TDirective.newBuilder()
            .setVideoCallMuteMicDirective(directive)
            .build()
    }

    private fun convertTelegramMuteMicData(
        src: MuteMicDirective.TelegramMuteMicData,
        ctx: ToProtoContext
    ): CapabilityProto.TVideoCallCapability.TMuteMicDirective.TTelegramMuteMicData {
        return CapabilityProto.TVideoCallCapability.TMuteMicDirective.TTelegramMuteMicData.newBuilder().apply {
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

    override val directiveType: Class<MuteMicDirective>
        get() = MuteMicDirective::class.java
}
