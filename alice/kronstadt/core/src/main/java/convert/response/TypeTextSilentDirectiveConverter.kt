package ru.yandex.alice.kronstadt.core.convert.response

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.directive.TypeTextSilentDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TTypeTextSilentDirective

@Component
open class TypeTextSilentDirectiveConverter : DirectiveConverterBase<TypeTextSilentDirective> {
    override fun convert(src: TypeTextSilentDirective, ctx: ToProtoContext): TDirective {
        val directive: TTypeTextSilentDirective.Builder = TTypeTextSilentDirective.newBuilder()
            .setName("external_skill__type_text_silent")
            .setText(src.text)
        return TDirective.newBuilder()
            .setTypeTextSilentDirective(directive)
            .build()
    }

    override val directiveType: Class<TypeTextSilentDirective>
        get() = TypeTextSilentDirective::class.java
}
