package ru.yandex.alice.kronstadt.core.layout

import ru.yandex.alice.kronstadt.core.convert.ProtoUtil
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoContext
import ru.yandex.alice.kronstadt.core.utils.append
import ru.yandex.alice.kronstadt.core.utils.prepend
import ru.yandex.alice.megamind.protos.scenarios.ResponseProto

interface Card {

    fun toProto(ctx: ToProtoContext, protoUtil: ProtoUtil): ResponseProto.TLayout.TCard

    interface Builder<T : Card> {
        fun build(): T
    }
}

class TextCard @JvmOverloads constructor(
    val text: String?,
    val buttons: List<Button> = listOf(),
) : Card {

    override fun toProto(ctx: ToProtoContext, protoUtil: ProtoUtil): ResponseProto.TLayout.TCard {
        val cardProto = ResponseProto.TLayout.TCard.newBuilder();
        if (buttons.isEmpty()) {
            cardProto.setText(text ?: "")
        } else {
            val buttons = buttons.map { convertButton(it, ctx) }
            val textWithButtons = ResponseProto.TLayout.TTextWithButtons.newBuilder()
                .setText(text ?: "")
                .addAllButtons(buttons)
                .build()
            cardProto.textWithButtons = textWithButtons
        }
        return cardProto.build()
    }

    private fun convertButton(src: Button, ctx: ToProtoContext): ResponseProto.TLayout.TButton {
        return ResponseProto.TLayout.TButton.newBuilder()
            .setTitle(src.text)
            .setActionId(ctx.rewriteAction(src))
            .build()
    }

    fun toBuilder(): Builder {
        return Builder(text, buttons)
    }

    class Builder(
        private var text: String? = null,
        buttons: List<Button> = listOf(),
    ) : Card.Builder<TextCard> {

        var buttons: MutableList<Button> = buttons.toMutableList()

        fun isEmpty(): Boolean {
            return text == null && buttons.isEmpty()
        }

        fun setText(text: String): Builder {
            this.text = text
            return this
        }

        fun getText(): String? {
            return text
        }

        fun prependText(prefix: String, separator: String = " ", capitalize: Boolean = true): Builder {
            this.text = this.text.prepend(prefix, separator, capitalize)
            return this
        }

        fun appendText(suffix: String?, separator: String = " "): Builder {
            this.text = this.text.append(suffix, separator)
            return this
        }

        fun buttons(buttons: List<Button>): Builder {
            this.buttons = buttons.toMutableList()
            return this
        }

        fun button(button: Button): Builder {
            this.buttons.add(button)
            return this
        }

        override fun build(): TextCard {
            return TextCard(text, buttons)
        }
    }

    companion object {
        @JvmStatic
        fun builder(): Builder {
            return Builder()
        }
    }
}
