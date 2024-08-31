package ru.yandex.alice.kronstadt.core.layout.div.block

import com.fasterxml.jackson.annotation.JsonInclude
import com.fasterxml.jackson.annotation.JsonProperty
import ru.yandex.alice.kronstadt.core.layout.div.DivAction
import ru.yandex.alice.kronstadt.core.layout.div.DivElement
import ru.yandex.alice.kronstadt.core.layout.div.Size
import ru.yandex.alice.kronstadt.core.layout.div.TextStyle
import java.util.Optional

@JsonInclude(JsonInclude.Include.NON_ABSENT)
class UniversalBlock internal constructor(
    val title: String? = null,
    val text: String? = null,
    @JsonProperty("text_max_lines") val textMaxLines: Int? = null,
    @JsonProperty("title_max_lines") val titleMaxLines: Int? = null,
    @JsonProperty("title_style") val titleStyle: TextStyle? = null,
    @JsonProperty("text_style") val textStyle: TextStyle? = null,
    @JsonProperty("side_element") val sideElement: SideElement? = null,
    override val action: DivAction? = null
) : Block {

    override val type: BlockType
        get() = BlockType.UNIVERSAL

    data class SideElement(
        val element: DivElement,
        val size: Size,
        val position: Position,
    )

    class UniversalBlockBuilder internal constructor() {
        private var title: String? = null
        private var text: String? = null
        private var textMaxLines: Int? = null
        private var titleMaxLines: Int? = null
        private var titleStyle: TextStyle? = null
        private var textStyle: TextStyle? = null
        private var sideElement: SideElement? = null
        private var action: DivAction? = null
        fun title(title: String?): UniversalBlockBuilder {
            this.title = title
            return this
        }

        fun text(text: String?): UniversalBlockBuilder {
            this.text = text
            return this
        }

        fun textMaxLines(textMaxLines: Int?): UniversalBlockBuilder {
            this.textMaxLines = textMaxLines
            return this
        }

        fun titleMaxLines(titleMaxLines: Int?): UniversalBlockBuilder {
            this.titleMaxLines = titleMaxLines
            return this
        }

        fun titleStyle(titleStyle: TextStyle?): UniversalBlockBuilder {
            this.titleStyle = titleStyle
            return this
        }

        fun textStyle(textStyle: TextStyle?): UniversalBlockBuilder {
            this.textStyle = textStyle
            return this
        }

        fun sideElement(sideElement: SideElement?): UniversalBlockBuilder {
            this.sideElement = sideElement
            return this
        }

        fun action(action: DivAction?): UniversalBlockBuilder {
            this.action = action
            return this
        }

        fun build(): UniversalBlock {
            return UniversalBlock(
                title, text, textMaxLines, titleMaxLines, titleStyle, textStyle, sideElement, action
            )
        }
    }

    companion object {
        @JvmStatic
        fun withTitleOnly(title: Optional<String?>, titleStyle: TextStyle): UniversalBlock {
            return builder()
                .title(title.orElse(null))
                .titleStyle(titleStyle)
                .build()
        }

        @JvmStatic
        fun builder(): UniversalBlockBuilder {
            return UniversalBlockBuilder()
        }

        fun withTitleAndText(title: Optional<String?>, text: Optional<String?>): UniversalBlock {
            return builder()
                .title(title.orElse(null))
                .text(text.orElse(null))
                .build()
        }
    }
}
