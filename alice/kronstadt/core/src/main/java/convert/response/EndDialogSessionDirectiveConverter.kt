package ru.yandex.alice.kronstadt.core.convert.response

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.directive.EndDialogSessionDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TEndDialogSessionDirective

@Component
open class EndDialogSessionDirectiveConverter : DirectiveConverterBase<EndDialogSessionDirective> {
    override fun convert(src: EndDialogSessionDirective, ctx: ToProtoContext): TDirective {
        val directive = TEndDialogSessionDirective.newBuilder()
            .setName("external_skill__end_dialog_session")

        src.skillId?.also { skillId -> directive.dialogId = skillId }
        return TDirective.newBuilder()
            .setEndDialogSessionDirective(directive)
            .build()
    }

    override val directiveType: Class<EndDialogSessionDirective>
        get() = EndDialogSessionDirective::class.java
}
