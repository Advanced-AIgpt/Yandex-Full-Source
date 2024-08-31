package ru.yandex.alice.kronstadt.core.layout.div.block

import com.fasterxml.jackson.annotation.JsonInclude
import com.fasterxml.jackson.annotation.JsonProperty
import ru.yandex.alice.kronstadt.core.layout.div.DivAction

@JsonInclude(JsonInclude.Include.NON_ABSENT)
class GalleryBlock private constructor(
    @JsonProperty val items: List<ContainerBlock>,
    @JsonProperty("padding_between_items") val paddingBetweenItems: NumericDivSize? = null
) : Block {

    override val action: DivAction?
        get() = null

    override val type: BlockType
        get() = BlockType.GALLERY

    override val subBlocks: Collection<Block>
        get() = items

    companion object {

        @JvmStatic
        @JvmOverloads
        fun create(items: List<ContainerBlock>, paddingBetweenItems: NumericDivSize? = null): GalleryBlock {
            return GalleryBlock(items, paddingBetweenItems)
        }
    }
}
