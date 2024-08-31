package ru.yandex.alice.kronstadt.core.convert.response

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.convert.ProtoUtil
import ru.yandex.alice.kronstadt.core.directive.HideViewDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.THideViewDirective

@Component
class HideViewDirectiveConverter(
    val protoUtil: ProtoUtil
) : DirectiveConverterBase<HideViewDirective> {
    override val directiveType: Class<HideViewDirective>
        get() = HideViewDirective::class.java

    override fun convert(src: HideViewDirective, ctx: ToProtoContext): TDirective {
        val directive = THideViewDirective.newBuilder()
            .setName("hide_view")
            .setLayer(getLayer(src.layer))
            .build()
        return TDirective.newBuilder()
            .setHideViewDirective(directive)
            .build()
    }

    private fun getLayer(layer: HideViewDirective.Layer): THideViewDirective.TLayer {
        val layerBuilder = THideViewDirective.TLayer
            .newBuilder()
        when (layer) {
            is HideViewDirective.Dialog -> {
                layerBuilder.dialog = THideViewDirective.TLayer.TDialogLayer
                    .newBuilder()
                    .build()
            }
            is HideViewDirective.Content -> {
                layerBuilder.content = THideViewDirective.TLayer.TContentLayer
                    .newBuilder()
                    .build()
            }
            is HideViewDirective.Alarm -> {
                layerBuilder.alarm = THideViewDirective.TLayer.TAlarmLayer
                    .newBuilder()
                    .build()
            }
        }
        return layerBuilder.build()
    }
}
