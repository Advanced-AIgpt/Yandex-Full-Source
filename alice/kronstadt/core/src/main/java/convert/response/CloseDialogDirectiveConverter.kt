package ru.yandex.alice.kronstadt.core.convert.response

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.directive.CloseDialogDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TCloseDialogDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TDirective

@Component
open class CloseDialogDirectiveConverter : DirectiveConverterBase<CloseDialogDirective> {
    override fun convert(src: CloseDialogDirective, ctx: ToProtoContext): TDirective {
        val directive = TCloseDialogDirective.newBuilder()
            .setName("external_skill__close_dialog")
            .setDialogId(src.skillId)

        return TDirective.newBuilder()
            .setCloseDialogDirective(directive)
            .build()
    }

    override val directiveType: Class<CloseDialogDirective>
        get() = CloseDialogDirective::class.java
}
