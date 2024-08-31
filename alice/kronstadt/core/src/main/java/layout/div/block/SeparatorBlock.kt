package ru.yandex.alice.kronstadt.core.layout.div.block

import com.fasterxml.jackson.annotation.JsonFormat
import com.fasterxml.jackson.annotation.JsonInclude
import com.fasterxml.jackson.annotation.JsonProperty
import ru.yandex.alice.kronstadt.core.layout.div.DivAction
import ru.yandex.alice.kronstadt.core.layout.div.Size

@JsonInclude(JsonInclude.Include.NON_ABSENT)
class SeparatorBlock(
    val size: Size,
    @JsonFormat(shape = JsonFormat.Shape.NUMBER_INT)
    @JsonProperty("has_delimiter") val hasDelimiter: Boolean?,
    override val action: DivAction?
) : Block {

    override val type: BlockType
        get() = BlockType.SEPARATOR

    companion object {
        @JvmStatic
        fun withDelimiter(size: Size): SeparatorBlock {
            return SeparatorBlock(size, true, null)
        }

        @JvmStatic
        fun withoutDelimiter(size: Size): SeparatorBlock {
            return SeparatorBlock(size, null, null)
        }
    }
}
