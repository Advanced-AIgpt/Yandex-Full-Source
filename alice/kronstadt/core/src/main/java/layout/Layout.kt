package ru.yandex.alice.kronstadt.core.layout

import ru.yandex.alice.kronstadt.core.directive.MegaMindDirective
import ru.yandex.alice.kronstadt.core.layout.div.DivBody
import java.util.Optional

data class Layout(
    val outputSpeech: String?,
    val shouldListen: Boolean,
    val suggests: List<Button> = listOf(),
    val directives: List<MegaMindDirective> = listOf(),
    val cards: List<Card> = listOf(),
    val contentProperties: ContentProperties? = null
) {

    /**
     * Некоторые сервисы (например, биллинг и консоль разработчика) не поддерживают ответы с несколькими карточками
     * TODO: добавить поддержку нескольких текстовых карточек, удалить этот метод
     */
    fun joinTextCards(): TextCard? {
        val textCards: List<TextCard> = cards.filterIsInstance<TextCard>()
        if (textCards.isEmpty()) {
            return null
        } else {
            val firstCardButtons = textCards.get(0).buttons
            return TextCard(
                textCards.map { it.text }.joinToString(separator = "\n"),
                firstCardButtons,
            )
        }
    }

    fun getDivCards(): List<DivBody> {
        return cards.filterIsInstance<DivBody>()
    }

    fun getTextCards(): List<TextCard> {
        return cards.filterIsInstance<TextCard>()
    }

    fun withSuggests(newSuggests: List<Button>): Layout {
        return this.copy(suggests = newSuggests)
    }

    fun getOutputSpeechO() = Optional.ofNullable(outputSpeech)

    class Builder(
        var outputSpeech: String? = null,
        var shouldListen: Boolean = true,
        var suggests: MutableList<Button> = mutableListOf(),
        var directives: MutableList<MegaMindDirective> = mutableListOf(),
        var cards: MutableList<Card> = mutableListOf(),
        var contentProperties: ContentProperties? = null,
    ) {

        fun outputSpeech(outputSpeech: String?): Builder {
            this.outputSpeech = outputSpeech
            return this
        }

        fun shouldListen(shouldListen: Boolean): Builder {
            this.shouldListen = shouldListen
            return this
        }

        fun suggests(suggests: MutableList<Button>): Builder {
            this.suggests = suggests
            return this
        }

        fun suggest(suggest: Button): Builder {
            this.suggests.add(suggest)
            return this
        }

        fun directives(directives: MutableList<MegaMindDirective>): Builder {
            this.directives = directives
            return this
        }

        fun directive(directive: MegaMindDirective): Builder {
            this.directives.add(directive)
            return this
        }

        fun cards(cards: List<Card>): Builder {
            this.cards = cards.toMutableList()
            return this
        }

        fun addCards(cards: List<Card>): Builder {
            this.cards.addAll(cards)
            return this
        }

        fun textCard(textCard: TextCard): Builder {
            this.cards.add(textCard)
            return this
        }

        @JvmOverloads
        fun textCard(text: String, buttons: List<Button> = listOf()): Builder {
            this.cards.add(TextCard(text, buttons))
            return this
        }

        fun divCard(divCard: DivBody): Builder {
            this.cards.add(divCard)
            return this
        }

        fun hasDivCards(): Boolean {
            return cards.filterIsInstance<DivBody>().isNotEmpty()
        }

        fun contentProperties(contentProperties: ContentProperties?): Builder {
            this.contentProperties = contentProperties
            return this
        }

        fun build() = Layout(
            outputSpeech = outputSpeech,
            shouldListen = shouldListen,
            suggests = suggests,
            directives = directives,
            cards = cards,
            contentProperties = contentProperties?.takeIf {
                it.containsSensitiveDataInRequest ||
                    it.containsSensitiveDataInResponse
            }
        )
    }

    companion object {
        @JvmStatic
        fun builder(): Builder {
            return Builder()
        }

        @JvmStatic
        fun cardBuilder(outputSpeech: String?, shouldListen: Boolean, divBody: DivBody): Builder {
            return Builder()
                .outputSpeech(outputSpeech)
                .shouldListen(shouldListen)
                .divCard(divBody)
        }

        @JvmStatic
        fun cardBuilder(outputSpeech: String?, shouldListen: Boolean, vararg divBodies: Card): Builder {
            return Builder()
                .cards(divBodies.toList())
                .outputSpeech(outputSpeech)
                .shouldListen(shouldListen)
        }

        @JvmStatic
        @JvmOverloads
        fun textLayout(
            text: String,
            outputSpeech: String?,
            shouldListen: Boolean = false,
            suggests: List<Button> = listOf(),
            directives: List<MegaMindDirective> = listOf(),
        ) = Layout(
            cards = listOf(TextCard(text)),
            outputSpeech = outputSpeech,
            shouldListen = shouldListen,
            suggests = suggests,
            directives = directives,
        )

        @JvmStatic
        @JvmOverloads
        fun textLayout(
            texts: List<String>,
            outputSpeech: String?,
            shouldListen: Boolean = false,
            directives: List<MegaMindDirective> = emptyList(),
        ) = Layout(
            cards = texts.map { TextCard(it) },
            outputSpeech = outputSpeech,
            shouldListen = shouldListen,
            directives = directives,
        )

        @JvmStatic
        @JvmOverloads
        fun textWithOutputSpeech(
            textWithTts: TextWithTts,
            shouldListen: Boolean = false,
            directives: List<MegaMindDirective> = emptyList()
        ) = Layout(
            cards = listOf(TextCard(textWithTts.text)),
            outputSpeech = textWithTts.tts,
            shouldListen = shouldListen,
            directives = directives
        )

        @JvmStatic
        fun silence() = Layout(outputSpeech = null, shouldListen = false)

        @JvmStatic
        fun silentText(text: String) =
            Layout(outputSpeech = null, shouldListen = false, cards = listOf(TextCard(text)))

        @JvmStatic
        fun directiveOnlyLayout(directives: List<MegaMindDirective>) =
            Layout(outputSpeech = null, shouldListen = false, directives = directives)

        @JvmStatic
        fun directiveOnlyLayout(vararg directives: MegaMindDirective) =
            Layout(outputSpeech = null, shouldListen = false, directives = directives.toList())
    }
}


