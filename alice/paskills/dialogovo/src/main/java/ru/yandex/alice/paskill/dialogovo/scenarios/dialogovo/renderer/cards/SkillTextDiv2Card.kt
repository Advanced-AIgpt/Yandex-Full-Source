package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.renderer.cards

import com.yandex.div.dsl.context.CardContext
import com.yandex.div.dsl.context.CardWithTemplates
import com.yandex.div.dsl.model.DivAlignmentHorizontal
import com.yandex.div.dsl.model.DivAlignmentVertical
import com.yandex.div.dsl.model.DivContainer
import com.yandex.div.dsl.model.DivFontWeight
import com.yandex.div.dsl.model.DivGallery
import com.yandex.div.dsl.model.DivImage
import com.yandex.div.dsl.model.DivImageScale
import com.yandex.div.dsl.model.DivSizeUnit
import com.yandex.div.dsl.model.DivText
import com.yandex.div.dsl.model.divAction
import com.yandex.div.dsl.model.divBorder
import com.yandex.div.dsl.model.divContainer
import com.yandex.div.dsl.model.divData
import com.yandex.div.dsl.model.divEdgeInsets
import com.yandex.div.dsl.model.divFixedSize
import com.yandex.div.dsl.model.divGallery
import com.yandex.div.dsl.model.divGradientBackground
import com.yandex.div.dsl.model.divImage
import com.yandex.div.dsl.model.divMatchParentSize
import com.yandex.div.dsl.model.divSolidBackground
import com.yandex.div.dsl.model.divText
import com.yandex.div.dsl.model.divWrapContentSize
import com.yandex.div.dsl.model.state
import com.yandex.div.dsl.type.BoolInt
import com.yandex.div.dsl.type.color
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.domain.DialogovoSkillCardData
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.domain.SkillInfoData
import java.net.URI

object SkillTextDiv2Card {
    fun card(
        scenarioData: DialogovoSkillCardData,
    ): CardWithTemplates = CardWithTemplates(card = com.yandex.div.dsl.context.card {
        divData(
            logId = "dialog_response", states = listOf(
                state(
                    stateId = 0,
                    div = divContainer(
                        background = listOf(
                            divSolidBackground("#121316".color)
                        ),
                        height = divMatchParentSize(),
                        width = divMatchParentSize(),
                        orientation = DivContainer.Orientation.OVERLAP,
                        items = listOf(
                            content(scenarioData),
                            header(scenarioData.skillInfo),
                            suggestsGallery(scenarioData.response.suggests),
                        )
                    )
                )
            )
        )
    })

    fun CardContext.content(
        data: DialogovoSkillCardData,
    ): DivGallery = divGallery(
        height = divFixedSize(value = 880),
        paddings = divEdgeInsets(
            top = 184,
            bottom = 184,
        ),
        orientation = DivGallery.Orientation.VERTICAL,
        items = listOfNotNull(
            request(data.request),
            (data.response.card as? DialogovoSkillCardData.TextResponse)?.let { textResponse(it) }
                ?: throw NotImplementedError(""),
            skillButtonsGallery(data.response.buttons),
        )
    )

    fun CardContext.request(request: DialogovoSkillCardData.Request): DivText = divText(
        text = request.text,
        fontSize = 40,
        fontWeight = DivFontWeight.REGULAR,
        lineHeight = 56,
        margins = divEdgeInsets(
            left = 48,
            right = 136,
            bottom = 10,
        ),
        textColor = "#80ffffff".color,
    )

    fun CardContext.textResponse(response: DialogovoSkillCardData.TextResponse): DivText = divText(
        text = response.text,
        alignmentHorizontal = DivAlignmentHorizontal.LEFT,
        alignmentVertical = DivAlignmentVertical.TOP,
        fontSize = 64,
        fontWeight = DivFontWeight.MEDIUM,
        height = divWrapContentSize(),
        margins = divEdgeInsets(
            left = 48,
            right = 136,
        ),
        textColor = "#fff".color,
    )

    fun CardContext.bigImageCardResponse(response: DialogovoSkillCardData.TextResponse): DivText = divText(
        text = response.text,
        alignmentHorizontal = DivAlignmentHorizontal.LEFT,
        alignmentVertical = DivAlignmentVertical.TOP,
        fontSize = 64,
        fontWeight = DivFontWeight.MEDIUM,
        height = divWrapContentSize(),
        lineHeight = 76,
        margins = divEdgeInsets(
            left = 48,
            right = 136,
        ),
        textColor = "#fff".color,
        width = divWrapContentSize(),
    )

    fun CardContext.skillButtonsGallery(suggests: List<DialogovoSkillCardData.Button>): DivGallery = divGallery(
        itemSpacing = 24,
        items = suggests.map { skillButton(it) },
        margins = divEdgeInsets(
            top = 48
        ),
        paddings = divEdgeInsets(
            left = 48,
            right = 48,
        )
    )

    fun CardContext.skillButton(suggest: DialogovoSkillCardData.Button): DivContainer = divContainer(
        background = listOf(
            divSolidBackground(color = "#121316".color)
        ),
        border = divBorder(
            cornerRadius = 28
        ),
        height = divWrapContentSize(),
        orientation = DivContainer.Orientation.HORIZONTAL,
        paddings = divEdgeInsets(
            left = 40,
            top = 29,
            right = 40,
            bottom = 29,
        ),
        items = listOf(skillButtonText(suggest))
    )

    fun CardContext.skillButtonText(suggest: DialogovoSkillCardData.Button): DivText = divText(
        text = suggest.text,
        fontSize = 44,
        fontWeight = DivFontWeight.REGULAR,
        lineHeight = 52,
        action = divAction(
            url = URI.create(suggest.url),
            logId = "button_centaur_action",
        ),
        textColor = "#121316".color
    )

    fun CardContext.header(
        skillInfo: SkillInfoData
    ): DivContainer = divContainer(
        alignmentHorizontal = DivAlignmentHorizontal.LEFT,
        alignmentVertical = DivAlignmentVertical.TOP,
        background = listOf(
            divGradientBackground(
                angle = 270,
                colors = listOf(
                    "#e6000000".color,
                    "#a0050607".color,
                    "#00121316".color,
                )
            )
        ),
        contentAlignmentHorizontal = DivAlignmentHorizontal.CENTER,
        orientation = DivContainer.Orientation.HORIZONTAL,
        paddings = divEdgeInsets(
            left = 48,
            bottom = 48,
            top = 48,
            right = 48,
        ),
        items = listOf(
            logo(skillInfo.logo!!),
            headerText(skillInfo.name),
            //closeButton() use native close button
        )
    )

    fun CardContext.logo(
        logoUrl: String
    ): DivImage = divImage(
        imageUrl = URI.create(logoUrl),
        border = divBorder(
            cornerRadius = 44
        ),
        height = divFixedSize(value = 88, unit = DivSizeUnit.DP),
        width = divFixedSize(value = 88, unit = DivSizeUnit.DP),
        margins = divEdgeInsets(
            right = 32
        ),
    )

    fun CardContext.headerText(
        title: String
    ): DivText = divText(
        text = title,
        fontSize = 40,
        fontWeight = DivFontWeight.MEDIUM,
        lineHeight = 6,
        textColor = "#88898A".color,
    )

    fun CardContext.closeButton(): DivImage = divImage(
        imageUrl = URI.create("https://yastatic.net/s3/dialogs/smart_displays/icons/close_wt_border.png"),
        action = divAction(
            url = URI.create("centaur://local_command?local_commands=[{'command':'close_layer','layer':'ALICE'}]"),
            logId = "close_fullscreen",
        ),
        alignmentHorizontal = DivAlignmentHorizontal.RIGHT,
        background = listOf(
            divSolidBackground("#1affffff".color),
        ),
        border = divBorder(
            cornerRadius = 20,
        ),
        height = divFixedSize(value = 88, unit = DivSizeUnit.DP),
        paddings = divEdgeInsets(
            left = 22,
            bottom = 22,
            top = 22,
            right = 22,
        ),
        preloadRequired = BoolInt.TRUE,
        scale = DivImageScale.FILL,
        width = divFixedSize(value = 88, unit = DivSizeUnit.DP),
    )

    fun CardContext.suggestsGallery(suggests: List<DialogovoSkillCardData.Suggest>): DivGallery = divGallery(
        alignmentHorizontal = DivAlignmentHorizontal.LEFT,
        alignmentVertical = DivAlignmentVertical.BOTTOM,
        background = listOf(
            divGradientBackground(
                angle = 90,
                colors = listOf(
                    "#e6000000".color,
                    "#a0050607".color,
                    "#00121316".color
                ),
            ),
        ),
        itemSpacing = 24,
        items = suggests.map { suggestButton(it) },
        paddings = divEdgeInsets(
            left = 48,
            right = 48,
            bottom = 48,
        )
    )

    fun CardContext.suggestButton(suggest: DialogovoSkillCardData.Suggest): DivText = divText(
        text = suggest.text,
        fontSize = 32,
        fontWeight = DivFontWeight.REGULAR,
        height = divWrapContentSize(),
        width = divWrapContentSize(),
        lineHeight = 40,
        action = divAction(
            url = URI.create(suggest.url),
            logId = "suggest_centaur_action",
        ),
        background = listOf(
            divSolidBackground("#292A2D".color)
        ),
        border = divBorder(
            cornerRadius = 44
        ),
        paddings = divEdgeInsets(
            top = 24,
            right = 28,
            bottom = 24,
            left = 28,
        ),
        textColor = "#fff".color,
    )
}
