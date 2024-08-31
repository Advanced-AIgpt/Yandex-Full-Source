package ru.yandex.alice.kronstadt.core.layout.div.block

import com.fasterxml.jackson.annotation.JsonFormat
import com.fasterxml.jackson.annotation.JsonProperty
import ru.yandex.alice.kronstadt.core.layout.div.DivAction
import ru.yandex.alice.kronstadt.core.layout.div.DivAlignment

data class ButtonsBlock internal constructor(
    private val items: List<ButtonElement>,

    val alignment: DivAlignment = DivAlignment.left,

    @JsonFormat(shape = JsonFormat.Shape.NUMBER_INT)
    @JsonProperty("is_fullwidth")
    val fullwidth: Boolean,
) : Block {

    override val action: DivAction?
        get() = null

    override val type: BlockType
        get() = BlockType.BUTTONS

    data class ButtonElement internal constructor(
        val action: DivAction,

        val text: String?,
        //Картинка на кнопке слева
        val image: ImageBlock?,

        @JsonProperty("background_color")
        val backgroundColor: Int?,
    )
}
