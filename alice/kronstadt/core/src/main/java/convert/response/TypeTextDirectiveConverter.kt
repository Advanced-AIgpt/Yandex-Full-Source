package ru.yandex.alice.kronstadt.core.convert.response

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.directive.TypeTextDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TTypeTextDirective

@Component
open class TypeTextDirectiveConverter : DirectiveConverterBase<TypeTextDirective> {
    override fun convert(src: TypeTextDirective, ctx: ToProtoContext): TDirective {
        val directive: TTypeTextDirective.Builder = TTypeTextDirective.newBuilder()
            .setName("external_skill__type_text")
            .setText(src.text)
        return TDirective.newBuilder()
            .setTypeTextDirective(directive)
            .build()
    }

    override val directiveType: Class<TypeTextDirective>
        get() = TypeTextDirective::class.java
}
