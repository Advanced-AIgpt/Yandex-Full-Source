package ru.yandex.alice.kronstadt.core.convert.response

import org.apache.logging.log4j.LogManager
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.convert.ProtoUtil
import ru.yandex.alice.kronstadt.core.layout.Button
import ru.yandex.alice.kronstadt.core.layout.Layout
import ru.yandex.alice.kronstadt.core.layout.Theme
import ru.yandex.alice.megamind.protos.common.Privacy
import ru.yandex.alice.megamind.protos.scenarios.ResponseProto.TLayout
import ru.yandex.alice.megamind.protos.scenarios.ResponseProto.TLayout.TSuggest
import ru.yandex.alice.megamind.protos.scenarios.ResponseProto.TLayout.TSuggest.TActionButton

@Component
open class LayoutConverter(
    private val protoUtil: ProtoUtil,
    private val directiveConverter: DirectiveConverter
) : ToProtoConverter<Layout, TLayout> {

    override fun convert(src: Layout, ctx: ToProtoContext): TLayout {
        val builder = TLayout.newBuilder()
            .setShouldListen(src.shouldListen)
        src.cards.forEach {
            builder.addCards(it.toProto(ctx, protoUtil))
        }
        builder.outputSpeech = src.outputSpeech ?: ""
        val suggests = src.suggests.map { convertSuggest(it, ctx) }
        val directives = src.directives.map { directiveConverter.convert(it, ctx) }

        src.contentProperties?.apply {
            builder.setContentProperties(
                Privacy.TContentProperties.newBuilder()
                    .setContainsSensitiveDataInRequest(this.containsSensitiveDataInRequest)
                    .setContainsSensitiveDataInResponse(this.containsSensitiveDataInResponse)
            )
        }

        return builder
            .addAllSuggestButtons(suggests)
            .addAllDirectives(directives)
            .build()
    }

    private fun convertSuggest(src: Button, ctx: ToProtoContext): TSuggest {
        val suggestButtonBuilder = TActionButton
            .newBuilder()
            .setTitle(src.text)
            .setActionId(ctx.rewriteAction(src))
        src.theme?.also { theme: Theme ->
            suggestButtonBuilder.theme = TActionButton.TTheme.newBuilder()
                .setImageUrl(theme.imageUrl)
                .build()
        }
        return TSuggest.newBuilder()
            .setActionButton(suggestButtonBuilder.build())
            .build()
    }

    companion object {
        private val logger = LogManager.getLogger(LayoutConverter::class.java)
    }
}
