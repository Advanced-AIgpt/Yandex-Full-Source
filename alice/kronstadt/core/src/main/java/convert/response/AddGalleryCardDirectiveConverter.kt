package ru.yandex.alice.kronstadt.core.convert.response

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.convert.ProtoUtil
import ru.yandex.alice.kronstadt.core.directive.AddGalleryCardDirective
import ru.yandex.alice.kronstadt.core.domain.scenariodata.teasers.TeaserConfigData
import ru.yandex.alice.megamind.protos.scenarios.directive.TAddCardDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TDirective
import ru.yandex.alice.protos.data.scenario.centaur.teasers.TeaserSettings.TCentaurTeaserConfigData
import ru.yandex.alice.protos.div.Div2CardProto

@Component
open class AddGalleryCardDirectiveConverter(
    val protoUtil: ProtoUtil
) : DirectiveConverterBase<AddGalleryCardDirective> {

    override fun convert(src: AddGalleryCardDirective, ctx: ToProtoContext): TDirective {
        val directive = TAddCardDirective.newBuilder()
            .setName("external_skill__add_card")
            .setCardId(src.cardId)
            .setTeaserConfig(getTeaserConfig(src.teaserConfigData))
            .setCardShowTimeSec(src.showTime.toSeconds().toInt())
            .apply {
                if (src.div2Card != null) {
                    div2Card = Div2CardProto.TDiv2Card
                        .newBuilder()
                        .setBody(protoUtil.objectToStruct(src.div2Card.card))
                        .build()
                    div2Templates = protoUtil.objectToStruct(src.div2Card.templates)
                }
            }

        return TDirective.newBuilder()
            .setAddCardDirective(directive)
            .build()
    }

    override val directiveType: Class<AddGalleryCardDirective>
        get() = AddGalleryCardDirective::class.java

    private fun getTeaserConfig(teaserConfigData: TeaserConfigData): TCentaurTeaserConfigData {
        return TCentaurTeaserConfigData.newBuilder().setTeaserType(teaserConfigData.teaserType)
            .setTeaserId(teaserConfigData.teaserId).build()
    }
}
