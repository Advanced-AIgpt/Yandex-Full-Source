package ru.yandex.alice.kronstadt.core.layout.div.block

import com.fasterxml.jackson.annotation.JsonInclude
import ru.yandex.alice.kronstadt.core.layout.div.DivAction
import ru.yandex.alice.kronstadt.core.layout.div.ImageElement

@JsonInclude(JsonInclude.Include.NON_ABSENT)
class ImageBlock(val image: ImageElement, override val action: DivAction?) : Block {
    override val type: BlockType
        get() = BlockType.IMAGE

    companion object {
        @JvmStatic
        @JvmOverloads
        fun create(imageUrl: String, ratio: Double, action: DivAction? = null): ImageBlock {
            return ImageBlock(ImageElement(imageUrl, ratio), action)
        }
    }
}
