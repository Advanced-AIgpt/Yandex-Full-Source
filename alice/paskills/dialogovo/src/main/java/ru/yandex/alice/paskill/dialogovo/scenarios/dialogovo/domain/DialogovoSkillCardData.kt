package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.domain

import ru.yandex.alice.kronstadt.core.domain.scenariodata.ScenarioData

data class DialogovoSkillCardData(
    val skillInfo: SkillInfoData,
    val request: Request,
    val response: Response,
) : ScenarioData {

    data class Request(
        val text: String,
    )

    data class Response(
        val card: ResponseCard,
        val buttons: List<Button> = listOf(),
        val suggests: List<Suggest> = listOf(),
    )

    sealed class ResponseCard

    data class TextResponse(
        val text: String
    ) : ResponseCard()

    data class BigImageResponse(
        val image: ImageItem
    ) : ResponseCard()

    data class ImageGalleryResponse(
        val items: List<ImageItem>,
    ) : ResponseCard()

    data class ItemsListResponse(
        val header: ItemsListHeader?,
        val items: List<ImageItem>,
        val footer: ItemsListFooter?
    ) : ResponseCard()

    data class ItemsListHeader(
        val text: String,
    )

    data class ItemsListFooter(
        val text: String,
        val button: Button?,
    )

    data class ImageItem(
        val imageUrl: String,
        val title: String?,
        val description: String?,
        val button: Button?
    )

    data class Button(
        val text: String,
        val url: String,
        val payload: String? = null,
    )

    data class Suggest(
        val text: String,
        val url: String?,
        val payload: String? = null,
    )
}
