package ru.yandex.alice.paskill.dialogovo.vins

import com.fasterxml.jackson.annotation.JsonInclude
import com.fasterxml.jackson.annotation.JsonProperty
import ru.yandex.alice.paskill.dialogovo.external.v1.response.CardButton
import ru.yandex.alice.paskill.dialogovo.external.v1.response.CardType
import ru.yandex.alice.paskill.dialogovo.external.v1.response.ItemsListCardFooter
import ru.yandex.alice.paskill.dialogovo.external.v1.response.ItemsListCardHeader

sealed class VinsCard(val type: CardType)

@JsonInclude(JsonInclude.Include.NON_ABSENT)
data class BigImageVinsCard(
    @JsonProperty(value = "image_url") val imageId: String,
    val title: String?,
    val description: String?,
    val button: CardButton?
) : VinsCard(CardType.BIG_IMAGE)

@JsonInclude(JsonInclude.Include.NON_ABSENT)
data class ItemsListVinsCard(
    val items: List<ItemsListVinsItem>,
    val header: ItemsListCardHeader? = null,
    val footer: ItemsListCardFooter? = null,
) : VinsCard(CardType.ITEMS_LIST) {

    @JsonInclude(JsonInclude.Include.NON_ABSENT)
    data class ItemsListVinsItem(
        val title: String? = null,
        @JsonProperty(value = "image_url") val imageId: String? = null,
        val description: String? = null,
        val button: CardButton? = null,
    )
}

@JsonInclude(JsonInclude.Include.NON_ABSENT)
data class ImageGalleryVinsCard(
    val items: List<Item>,
    val header: ItemsListCardHeader? = null,
) : VinsCard(CardType.IMAGE_GALLERY) {

    @JsonInclude(JsonInclude.Include.NON_ABSENT)
    data class Item(
        @JsonProperty(value = "image_url")
        val imageUrl: String,
        val title: String? = null,
        val description: String? = null,
        val button: CardButton? = null,
    )

    data class ItemsListCardHeader(val text: String)
}
