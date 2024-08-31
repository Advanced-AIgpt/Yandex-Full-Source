package ru.yandex.alice.kronstadt.core.convert.response

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.directive.InternalThereminPlayDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TThereminPlayDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TThereminPlayDirective.TInternalSet

@Component
open class InternalThereminPlayDirectiveConverter : DirectiveConverterBase<InternalThereminPlayDirective> {
    override fun convert(src: InternalThereminPlayDirective, ctx: ToProtoContext): TDirective {
        val directive: TThereminPlayDirective.Builder = TThereminPlayDirective.newBuilder()
            .setInternalSet(TInternalSet.newBuilder().setMode(src.mode))
        return TDirective.newBuilder()
            .setThereminPlayDirective(directive)
            .build()
    }

    override val directiveType: Class<InternalThereminPlayDirective>
        get() = InternalThereminPlayDirective::class.java
}
