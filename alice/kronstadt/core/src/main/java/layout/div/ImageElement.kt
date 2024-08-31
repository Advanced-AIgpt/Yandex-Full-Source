package ru.yandex.alice.kronstadt.core.layout.div

import com.fasterxml.jackson.annotation.JsonProperty

data class ImageElement(
    @JsonProperty("image_url")
    val imageUrl: String,
    val ratio: Double,
) : DivElement {

    override val type: DivElementType
        get() = DivElementType.IMAGE
}
