package ru.yandex.alice.kronstadt.core.layout.div.block

import com.fasterxml.jackson.annotation.JsonInclude
import com.fasterxml.jackson.annotation.JsonProperty
import com.fasterxml.jackson.annotation.JsonValue
import ru.yandex.alice.kronstadt.core.layout.div.DivAction
import ru.yandex.alice.kronstadt.core.layout.div.DivAlignment
import ru.yandex.alice.kronstadt.core.layout.div.DivAlignmentVertical

@JsonInclude(JsonInclude.Include.NON_ABSENT)
class ContainerBlock(
    val direction: Direction,
    val width: DivSize,
    val height: DivSize,
    val children: List<Block> = listOf(),
    val frame: Frame? = null,
    @JsonProperty("alignment_horizontal") val alignmentHorizontal: DivAlignment? = null,
    @JsonProperty("alignment_vertical") val alignmentVertical: DivAlignmentVertical? = null,
    override val action: DivAction? = null,
) : Block {

    override val type: BlockType
        get() = BlockType.CONTAINER
    override val subBlocks: Collection<Block>
        get() = children

    enum class Direction(@JsonValue val code: String) {
        VERTICAL("vertical"),
        HORIZONTAL("horizontal");
    }

    @JsonInclude(JsonInclude.Include.NON_NULL)
    class Frame private constructor(@JsonProperty("style") val style: FrameStyle, val color: Int?) {

        companion object {
            private val SHADOW_INSTANCE = Frame(FrameStyle.SHADOW, null)
            private val CORNERS_INSTANCE = Frame(FrameStyle.ONLY_ROUND_CORNERS, null)

            @JvmStatic
            fun shadow(): Frame {
                return SHADOW_INSTANCE
            }

            @JvmStatic
            fun onlyRoundCorners(): Frame {
                return CORNERS_INSTANCE
            }

            @JvmStatic
            fun border(color: Int): Frame {
                return Frame(FrameStyle.BORDER, color)
            }
        }
    }

    enum class FrameStyle(@JsonValue val code: String) {
        BORDER("border"),
        SHADOW("shadow"),
        ONLY_ROUND_CORNERS("only_round_corners");
    }

    class ContainerBlockBuilder internal constructor() {
        private var children: MutableList<Block> = mutableListOf()
        private var direction: Direction? = null
        private var width: DivSize? = null
        private var height: DivSize? = null
        private var frame: Frame? = null
        private var alignmentHorizontal: DivAlignment? = null
        private var alignmentVertical: DivAlignmentVertical? = null
        private var action: DivAction? = null
        fun child(child: Block): ContainerBlockBuilder {
            children.add(child)
            return this
        }

        fun children(children: Collection<Block>): ContainerBlockBuilder {
            this.children.addAll(children)
            return this
        }

        fun clearChildren(): ContainerBlockBuilder {
            children.clear()
            return this
        }

        fun direction(direction: Direction): ContainerBlockBuilder {
            this.direction = direction
            return this
        }

        fun width(width: DivSize): ContainerBlockBuilder {
            this.width = width
            return this
        }

        fun height(height: DivSize): ContainerBlockBuilder {
            this.height = height
            return this
        }

        fun frame(frame: Frame?): ContainerBlockBuilder {
            this.frame = frame
            return this
        }

        fun alignmentHorizontal(alignmentHorizontal: DivAlignment?): ContainerBlockBuilder {
            this.alignmentHorizontal = alignmentHorizontal
            return this
        }

        fun alignmentVertical(alignmentVertical: DivAlignmentVertical?): ContainerBlockBuilder {
            this.alignmentVertical = alignmentVertical
            return this
        }

        fun action(action: DivAction?): ContainerBlockBuilder {
            this.action = action
            return this
        }

        fun build(): ContainerBlock {
            return ContainerBlock(
                direction!!,
                width!!,
                height!!,
                this.children.toList(),
                frame,
                alignmentHorizontal,
                alignmentVertical,
                action
            )
        }
    }

    companion object {
        @JvmStatic
        fun vertical(width: DivSize, height: DivSize): ContainerBlockBuilder {
            return ContainerBlockBuilder()
                .width(width)
                .height(height)
                .direction(Direction.VERTICAL)
        }

        fun builder(): ContainerBlockBuilder {
            return ContainerBlockBuilder()
        }

        fun horizontal(width: DivSize, height: DivSize): ContainerBlockBuilder {
            return ContainerBlockBuilder()
                .width(width)
                .height(height)
                .direction(Direction.HORIZONTAL)
        }
    }
}
