package ru.yandex.alice.kronstadt.core.convert.response

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.directive.GoBackNativeDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TGoBackwardDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TGoBackwardDirective.TNativeMode

@Component
open class GoBackNativeDirectiveConverter : DirectiveConverterBase<GoBackNativeDirective> {
    override val directiveType: Class<GoBackNativeDirective>
        get() = GoBackNativeDirective::class.java

    override fun convert(src: GoBackNativeDirective, ctx: ToProtoContext): TDirective {
        val directive = TGoBackwardDirective.newBuilder()
            .setNativeMode(TNativeMode.newBuilder().build())
            .build()
        return TDirective.newBuilder()
            .setGoBackwardDirective(directive)
            .build()
    }
}
