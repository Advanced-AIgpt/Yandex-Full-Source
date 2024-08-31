package ru.yandex.alice.kronstadt.core.convert.response

import org.springframework.context.annotation.Lazy
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.directive.OpenDialogDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TOpenDialogDirective

@Component
open class OpenDialogDirectiveConverter(
    @Lazy private val directiveConverter: DirectiveConverter
) : DirectiveConverterBase<OpenDialogDirective> {
    override fun convert(src: OpenDialogDirective, ctx: ToProtoContext): TDirective {
        val directives: List<TDirective> = src.directives
            .map { directiveConverter.convert(it, ctx) }

        val directive: TOpenDialogDirective = TOpenDialogDirective.newBuilder()
            .setName("external_skill__open_dialog")
            .setDialogId(src.skillId)
            .addAllDirectives(directives)
            .build()
        return TDirective.newBuilder()
            .setOpenDialogDirective(directive)
            .build()
    }

    override val directiveType: Class<OpenDialogDirective>
        get() = OpenDialogDirective::class.java
}
