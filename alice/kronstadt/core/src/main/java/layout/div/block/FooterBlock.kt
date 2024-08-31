package ru.yandex.alice.kronstadt.core.layout.div.block

import com.fasterxml.jackson.annotation.JsonInclude
import ru.yandex.alice.kronstadt.core.layout.div.DivAction

@JsonInclude(JsonInclude.Include.NON_ABSENT)
class FooterBlock(
    val text: String,
    override val action: DivAction? = null,
) : Block {
    override val type: BlockType
        get() = BlockType.FOOTER
}
