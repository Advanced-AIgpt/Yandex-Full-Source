package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.renderer

import com.yandex.div.dsl.context.CardWithTemplates
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.DivRenderData
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoContext
import ru.yandex.alice.kronstadt.core.input.UtteranceInput
import ru.yandex.alice.paskill.dialogovo.domain.AvatarsNamespace
import ru.yandex.alice.paskill.dialogovo.domain.Image.getImageUrl
import ru.yandex.alice.paskill.dialogovo.domain.ImageAlias
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessResult
import ru.yandex.alice.paskill.dialogovo.external.v1.response.BigImageCard
import ru.yandex.alice.paskill.dialogovo.external.v1.response.BigImageListCard
import ru.yandex.alice.paskill.dialogovo.external.v1.response.Button
import ru.yandex.alice.paskill.dialogovo.external.v1.response.CardButton
import ru.yandex.alice.paskill.dialogovo.external.v1.response.ItemsListCard
import ru.yandex.alice.paskill.dialogovo.external.v1.response.ItemsListItem
import ru.yandex.alice.paskill.dialogovo.external.v1.response.Response
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoScenarioDataConverter
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.domain.DialogovoSkillCardData
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.domain.SkillInfoData
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.renderer.cards.SkillTextDiv2Card
import java.util.Optional

@Component
class SkillDialogRendererService(
    private val scenarioDataConverter: DialogovoScenarioDataConverter,
) {

    private val avatarsUrl = "https://avatars.mds.yandex.net"
    private val skillDialogCardId = "dialogovo.skill.div.card"

    fun getSkillDialogCard(
        request: MegaMindRequest<DialogovoState>,
        processResult: SkillProcessResult
    ): CardWithTemplates? = getSkillDialogScenarioData(request, processResult)?.let {
        SkillTextDiv2Card.card(it)
    }

    fun getSkillDialogRenderData(
        request: MegaMindRequest<DialogovoState>,
        processResult: SkillProcessResult
    ): DivRenderData? = getSkillDialogScenarioData(request, processResult)?.let {
        DivRenderData(skillDialogCardId, scenarioDataConverter.convert(it, ToProtoContext()))
    }

    private fun getSkillDialogScenarioData(
        request: MegaMindRequest<DialogovoState>,
        processResult: SkillProcessResult
    ): DialogovoSkillCardData? {
        val webhookResponse = processResult.getResponseO().orElse(null)
            .response.orElse(null)
            .response.orElse(null) ?: return null
        val requestUtterance: String = if (request.input is UtteranceInput) {
            (request.input as UtteranceInput).originalUtterance
        } else ""

        return DialogovoSkillCardData(
            skillInfo = SkillInfoData(
                processResult.skill.name,
                processResult.skill.logoUrl,
                processResult.skill.id
            ),
            request = DialogovoSkillCardData.Request(requestUtterance),
            response = DialogovoSkillCardData.Response(
                card = getResponseCard(webhookResponse),
                buttons = getButtonsList(webhookResponse),
                suggests = getSuggestsList(webhookResponse)
            )
        )
    }

    private fun getButtonsList(webhookResponse: Response): List<DialogovoSkillCardData.Button> {
        if (!webhookResponse.buttons.isPresent) {
            return listOf()
        }
        return webhookResponse.buttons.get().filter { !it.hide }
            .map { DialogovoSkillCardData.Button(it.title, getUrl(it), it.payload) }
    }

    private fun getSuggestsList(webhookResponse: Response): List<DialogovoSkillCardData.Suggest> {
        if (!webhookResponse.buttons.isPresent) {
            return listOf()
        }
        return webhookResponse.buttons.get().filter { it.hide }
            .map { DialogovoSkillCardData.Suggest(it.title, getUrl(it), it.payload) }
    }

    private fun getUrl(button: Button): String =
        button.url.orElse("dialog://text_command?query=${button.title}")

    private fun getButton(button: CardButton, imageTitle: Optional<String>): DialogovoSkillCardData.Button {
        val text = button.text.orElse(imageTitle.orElse(""))
        return DialogovoSkillCardData.Button(
            text,
            button.url.orElse("dialog://text_command?query=$text"),
            button.payload
        )
    }

    private fun getResponseCard(webhookResponse: Response): DialogovoSkillCardData.ResponseCard {
        if (!webhookResponse.card.isPresent) {
            return DialogovoSkillCardData.TextResponse(webhookResponse.text.orEmpty())
        }

        return when (val responseCard = webhookResponse.card.get()) {
            is BigImageCard -> {
                val namespace: AvatarsNamespace = responseCard.mdsNamespace
                    .map { name: String -> AvatarsNamespace.createCustom(name) }
                    .orElse(AvatarsNamespace.DIALOGS_SKILL_CARD)
                return DialogovoSkillCardData.BigImageResponse(
                    DialogovoSkillCardData.ImageItem(
                        imageUrl = getImageUrl(responseCard.imageId, namespace, ImageAlias.ORIG)!!,
                        title = responseCard.title.orElse(null),
                        description = responseCard.description.orElse(null),
                        button = responseCard.button.map { getButton(it, responseCard.title) }.orElse(null)
                    )
                )
            }
            is ItemsListCard -> {
                return DialogovoSkillCardData.ItemsListResponse(
                    header = responseCard.header.map { DialogovoSkillCardData.ItemsListHeader(it.text.orElseThrow()) }
                        .orElse(null),
                    items = responseCard.items.map { getImageItem(it) },
                    footer = responseCard.footer.map { cardFooter ->
                        DialogovoSkillCardData.ItemsListFooter(
                            cardFooter.text.orElseThrow(),
                            cardFooter.button.map { getButton(it, cardFooter.text) }.orElse(null)
                        )
                    }.orElse(null)
                )
            }
            is BigImageListCard -> DialogovoSkillCardData.ImageGalleryResponse(
                items = responseCard.items.map { getImageItem(it) }
            )
            else -> throw RuntimeException()
        }
    }

    private fun getImageItem(imageItem: BigImageListCard.Item): DialogovoSkillCardData.ImageItem {
        val namespace: AvatarsNamespace = imageItem.mdsNamespace
            .map { name: String -> AvatarsNamespace.createCustom(name) }
            .orElse(AvatarsNamespace.DIALOGS_SKILL_CARD)
        return DialogovoSkillCardData.ImageItem(
            imageUrl = getImageUrl(imageItem.imageId, namespace, ImageAlias.ORIG),
            title = imageItem.title.orElse(null),
            description = imageItem.description.orElse(null),
            button = imageItem.button.map { getButton(it, imageItem.title) }.orElse(null)
        )
    }

    private fun getImageItem(imageItem: ItemsListItem): DialogovoSkillCardData.ImageItem {
        val namespace: AvatarsNamespace = imageItem.mdsNamespace
            .map { name: String -> AvatarsNamespace.createCustom(name) }
            .orElse(AvatarsNamespace.DIALOGS_SKILL_CARD)
        return DialogovoSkillCardData.ImageItem(
            imageUrl = getImageUrl(imageItem.imageId.get(), namespace, ImageAlias.ORIG),
            title = imageItem.title.orElse(null),
            description = imageItem.description.orElse(null),
            button = imageItem.button.map { getButton(it, imageItem.title) }.orElse(null)
        )
    }

    private fun getImageUrl(imageId: String?, namespace: AvatarsNamespace?, alias: ImageAlias?): String {
        return getImageUrl(imageId!!, namespace!!, alias!!, avatarsUrl)
    }
}
