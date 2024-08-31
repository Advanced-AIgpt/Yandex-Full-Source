package ru.yandex.alice.kronstadt.core.convert.response

import com.google.protobuf.Struct
import com.google.protobuf.util.JsonFormat
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.convert.ProtoUtil
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrameRequestData
import ru.yandex.alice.megamind.protos.common.Atm
import ru.yandex.alice.megamind.protos.common.FrameProto

@Component
open class SemanticFrameRequestDataConverter(
    private val protoUtil: ProtoUtil
) : ToProtoConverter<SemanticFrameRequestData, FrameProto.TSemanticFrameRequestData> {

    override fun convert(src: SemanticFrameRequestData, ctx: ToProtoContext): FrameProto.TSemanticFrameRequestData {
        val frameProto = FrameProto.TTypedSemanticFrame.newBuilder()
        src.typedSemanticFrame.writeTypedSemanticFrame(frameProto)
        return FrameProto.TSemanticFrameRequestData
            .newBuilder()
            .setTypedSemanticFrame(frameProto.build())
            .setAnalytics(convertAnalytics(src.analytics))
            .build()
    }

    fun convertToStruct(src: SemanticFrameRequestData, ctx: ToProtoContext): Struct {
        val semanticFrameRequestDatProto = this.convert(src, ctx)
        return protoUtil.jsonStringToStruct(JsonFormat.printer().print(semanticFrameRequestDatProto))
    }

    private fun convertAnalytics(
        analytics: SemanticFrameRequestData.AnalyticsTrackingModule
    ): Atm.TAnalyticsTrackingModule {
        return Atm.TAnalyticsTrackingModule
            .newBuilder()
            .setProductScenario(analytics.productScenario)
            .setOrigin(Atm.TAnalyticsTrackingModule.EOrigin.Scenario)
            .setPurpose(analytics.purpose)
            .build()
    }
}
