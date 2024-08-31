package ru.yandex.alice.kronstadt.core.convert.response

import com.yandex.div.dsl.serializer.toJsonNode
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.convert.ProtoUtil
import ru.yandex.alice.kronstadt.core.directive.ShowViewDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TShowViewDirective
import ru.yandex.alice.protos.div.Div2CardProto

@Component
class ShowViewDirectiveConverter(
    val protoUtil: ProtoUtil
) : DirectiveConverterBase<ShowViewDirective> {

    override fun convert(src: ShowViewDirective, ctx: ToProtoContext): TDirective {
        val directive = TShowViewDirective.newBuilder()
            .setName("external_skill__show_view")
            .setDoNotShowCloseButton(src.doNotShowCloseButton)
            .setLayer(convertLayer(src.layer))
            .setInactivityTimeout(convertInactivityTimeout(src.inactivityTimeout))

        src.actionSpaceId?.apply {
            directive.actionSpaceId = src.actionSpaceId
        }

        when (src.card) {
            is ShowViewDirective.Div2Card -> directive.div2Card = Div2CardProto.TDiv2Card
                .newBuilder()
                .setBody(protoUtil.objectToStruct(src.card.templateDivCard.toJsonNode()))
                .build()
            is ShowViewDirective.CardId -> directive.cardId = src.card.id
        }

        return TDirective.newBuilder()
            .setShowViewDirective(directive)
            .build()
    }

    private fun convertLayer(layer: ShowViewDirective.Layer): TShowViewDirective.TLayer {
        val layerBuilder = TShowViewDirective.TLayer
            .newBuilder()
        when (layer) {
            is ShowViewDirective.Dialog -> {
                layerBuilder.dialog = TShowViewDirective.TLayer.TDialogLayer
                    .newBuilder()
                    .build()
            }
            is ShowViewDirective.Content -> {
                layerBuilder.content = TShowViewDirective.TLayer.TContentLayer
                    .newBuilder()
                    .build()
            }
            is ShowViewDirective.Alarm -> {
                layerBuilder.alarm = TShowViewDirective.TLayer.TAlarmLayer
                    .newBuilder()
                    .build()
            }
        }
        return layerBuilder.build()
    }

    private fun convertInactivityTimeout(
        type: ShowViewDirective.InactivityTimeout
    ): TShowViewDirective.EInactivityTimeout {
        return when (type) {
            ShowViewDirective.InactivityTimeout.SHORT -> TShowViewDirective.EInactivityTimeout.Short
            ShowViewDirective.InactivityTimeout.MEDIUM -> TShowViewDirective.EInactivityTimeout.Medium
            ShowViewDirective.InactivityTimeout.LONG -> TShowViewDirective.EInactivityTimeout.Long
            ShowViewDirective.InactivityTimeout.INFINITY -> TShowViewDirective.EInactivityTimeout.Infinity
        }
    }

    override val directiveType: Class<ShowViewDirective>
        get() = ShowViewDirective::class.java
}
