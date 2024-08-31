package ru.yandex.alice.kronstadt.core.convert.response;

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.directive.GoHomeDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TGoHomeDirective

@Component
open class GoHomeDirectiveConverter : DirectiveConverterBase<GoHomeDirective> {

    override val directiveType: Class<GoHomeDirective>
        get() = GoHomeDirective::class.java

    override fun convert(src: GoHomeDirective, ctx: ToProtoContext): TDirective {
        val directive = TGoHomeDirective.newBuilder().build()
        return TDirective.newBuilder()
            .setGoHomeDirective(directive)
            .build()
    }
}
