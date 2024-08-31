package ru.yandex.alice.kronstadt.core.convert.response

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.directive.OpenUriDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TOpenUriDirective

@Component
open class OpenUriDirectiveConverter : DirectiveConverterBase<OpenUriDirective> {
    override fun convert(src: OpenUriDirective, ctx: ToProtoContext): TDirective {
        val directive: TOpenUriDirective.Builder = TOpenUriDirective.newBuilder()
            .setName("external_skill__open_uri")
            .setUri(src.uri)
        return TDirective.newBuilder()
            .setOpenUriDirective(directive)
            .build()
    }

    override val directiveType: Class<OpenUriDirective>
        get() = OpenUriDirective::class.java
}
