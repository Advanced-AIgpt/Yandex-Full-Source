package ru.yandex.alice.kronstadt.core.convert.response

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.directive.CallbackDirective
import ru.yandex.alice.kronstadt.core.directive.MegaMindDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TDirective

@Component
class DirectiveConverterImpl(
    converters: List<DirectiveConverterBase<out MegaMindDirective>>,
    private val callbackDirectiveConverter: CallbackDirectiveConverter
) : DirectiveConverter {
    @Suppress("UNCHECKED_CAST")
    private val convertersToType: Map<Class<out MegaMindDirective>, DirectiveConverterBase<MegaMindDirective>> =
        converters.associate { it.directiveType to it as DirectiveConverterBase<MegaMindDirective> }

    override fun convert(src: MegaMindDirective, ctx: ToProtoContext): TDirective {
        if (src is CallbackDirective) {
            return callbackDirectiveConverter.convert(src, ctx)
        }
        val c: Class<out MegaMindDirective> = src.javaClass
        val converter = convertersToType[c]
            ?: throw RuntimeException("Unable convert directive to proto ${src.javaClass.name}")
        return converter.convert(src, ctx)
    }
}
