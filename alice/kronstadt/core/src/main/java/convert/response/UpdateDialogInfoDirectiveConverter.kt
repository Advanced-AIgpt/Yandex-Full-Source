package ru.yandex.alice.kronstadt.core.convert.response

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.directive.Style
import ru.yandex.alice.kronstadt.core.directive.UpdateDialogInfoDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TUpdateDialogInfoDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TUpdateDialogInfoDirective.TMenuItem
import ru.yandex.alice.megamind.protos.scenarios.directive.TUpdateDialogInfoDirective.TStyle
import ru.yandex.alice.protos.extensions.ExtensionsProto

@Component
open class UpdateDialogInfoDirectiveConverter : DirectiveConverterBase<UpdateDialogInfoDirective> {
    private val defaultName: String =
        TUpdateDialogInfoDirective.getDescriptor().options.getExtension(ExtensionsProto.speechKitName)!!

    override fun convert(src: UpdateDialogInfoDirective, ctx: ToProtoContext): TDirective {
        val dialogInfo: TUpdateDialogInfoDirective = TUpdateDialogInfoDirective.newBuilder()
            .setName(src.name ?: defaultName)
            .setTitle(src.title)
            .setUrl(src.url)
            .addMenuItems(
                TMenuItem.newBuilder()
                    .setTitle("Описание навыка")
                    .setUrl(src.url)
            )
            .setImageUrl(src.imageUrl)
            .setStyle(getStyle(src.style))
            .setDarkStyle(getStyle(src.darkStyle))
            .setAdBlockId(src.adBlockId ?: "")
            .build()
        return TDirective.newBuilder()
            .setUpdateDialogInfoDirective(dialogInfo)
            .build()
    }

    private fun getStyle(style: Style): TStyle {
        return TStyle.newBuilder().apply {
            suggestBorderColor = style.suggestBorderColor
            userBubbleFillColor = style.userBubbleFillColor
            suggestTextColor = style.suggestTextColor
            suggestFillColor = style.suggestFillColor
            userBubbleTextColor = style.userBubbleTextColor
            skillActionsTextColor = style.skillActionsTextColor
            skillBubbleFillColor = style.skillBubbleFillColor
            skillBubbleTextColor = style.skillBubbleTextColor
            oknyxLogo = style.oknyxLogo
            addAllOknyxErrorColors(style.oknyxErrorColors)
            addAllOknyxNormalColors(style.oknyxNormalColors)
        }.build()
    }

    override val directiveType: Class<UpdateDialogInfoDirective>
        get() = UpdateDialogInfoDirective::class.java
}
