package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.converters

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoContext
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoConverter
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.domain.DialogovoSkillCardData
import ru.yandex.alice.protos.data.scenario.dialogovo.Skill.TDialogovoSkillCardData
import ru.yandex.alice.protos.data.scenario.dialogovo.Skill.TDialogovoSkillCardData.TSkillInfo
import ru.yandex.alice.protos.data.scenario.dialogovo.Skill.TDialogovoSkillCardData.TSkillRequest
import ru.yandex.alice.protos.data.scenario.dialogovo.Skill.TDialogovoSkillCardData.TSkillResponse

@Component
open class DialogovoSkillCardDataConverter : ToProtoConverter<DialogovoSkillCardData, TDialogovoSkillCardData> {

    override fun convert(src: DialogovoSkillCardData, ctx: ToProtoContext): TDialogovoSkillCardData {
        return TDialogovoSkillCardData.newBuilder()
            .setSkillInfo(
                TSkillInfo.newBuilder()
                    .setName(src.skillInfo.name)
                    .setLogo(src.skillInfo.logo)
                    .setSkillId(src.skillInfo.skillId)
            )
            .setSkillRequest(
                TSkillRequest.newBuilder()
                    .setText(src.request.text)
            )
            .setSkillResponse(convertResponse(src.response))
            .build()
    }

    private fun convertResponse(src: DialogovoSkillCardData.Response)
        : TSkillResponse {
        val response = TSkillResponse.newBuilder()
            .addAllButtons(src.buttons.map { convertButton(it) })
            .addAllSuggests(src.suggests.map { convertSuggest(it) })

        when (src.card) {
            is DialogovoSkillCardData.TextResponse -> response.textResponseBuilder.text = src.card.text
            is DialogovoSkillCardData.BigImageResponse -> response.bigImageResponseBuilder.imageItem = convertImageItem(src.card.image)
            is DialogovoSkillCardData.ImageGalleryResponse -> response.imageGalleryResponse = convertImageGalleryResponse(src.card)
            is DialogovoSkillCardData.ItemsListResponse -> response.itemsListResponse = convertItemsListResponse(src.card)
        }
        return response.build()
    }

    private fun convertImageGalleryResponse(src: DialogovoSkillCardData.ImageGalleryResponse)
        : TSkillResponse.TImageGalleryResponse {
        return TSkillResponse.TImageGalleryResponse.newBuilder()
            .addAllImageItems(src.items.map { convertImageItem(it) })
            .build()
    }

    private fun convertItemsListResponse(src: DialogovoSkillCardData.ItemsListResponse)
        : TSkillResponse.TItemsListResponse {
        return TSkillResponse.TItemsListResponse.newBuilder()
            .addAllImageItems(src.items.map { convertImageItem(it) })
            .apply {
                src.footer?.let { this.itemsLisetFooter = convertFooter(it) }
                src.header?.let { this.itemsLisetHeader = TSkillResponse.TItemsListHeader.newBuilder().setText(it.text).build() }
            }
            .build()
    }

    private fun convertImageItem(src: DialogovoSkillCardData.ImageItem)
        : TSkillResponse.TImageItem {
        return TSkillResponse.TImageItem.newBuilder()
            .apply {
                this.imageUrl = src.imageUrl
                src.title?.let { this.title = it }
                src.description?.let { this.description = it }
                src.button?.let { this.button = convertButton(it) }
            }.build()
    }

    private fun convertButton(src: DialogovoSkillCardData.Button)
        : TSkillResponse.TButton {
        val button = TSkillResponse.TButton.newBuilder()
            .setText(src.text)

        src.url.apply { button.url = src.url }
        src.payload?.apply { button.payload = src.payload }

        return button.build()
    }

    private fun convertSuggest(src: DialogovoSkillCardData.Suggest)
        : TSkillResponse.TSuggest {
        val suggest = TSkillResponse.TSuggest.newBuilder()
            .setText(src.text)

        src.url?.apply { suggest.url = src.url }
        src.payload?.apply { suggest.payload = src.payload }

        return suggest.build()
    }

    private fun convertFooter(src: DialogovoSkillCardData.ItemsListFooter)
        : TSkillResponse.TItemsListFooter {
        return TSkillResponse.TItemsListFooter.newBuilder()
            .setText(src.text)
            .apply { src.button?.let { button -> this.button = convertButton(button) } }
            .build()
    }
}
