package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.renderer

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.domain.scenariodata.ScenarioData
import ru.yandex.alice.paskill.dialogovo.domain.AvatarsNamespace
import ru.yandex.alice.paskill.dialogovo.domain.Image
import ru.yandex.alice.paskill.dialogovo.domain.ImageAlias
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessResult
import ru.yandex.alice.paskill.dialogovo.external.v1.response.WidgetGalleryMeta
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.domain.MainScreenSkillCardData
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.domain.SkillInfoData

@Component
class SkillWidgetRendererService {

    fun getMainScreenCardData(
        skillProcessResult: SkillProcessResult
    ): ScenarioData {
        if (!skillProcessResult.widgetItem.isPresent) {
            return MainScreenSkillCardData(
                SkillInfoData(
                    name = skillProcessResult.skill.name,
                    logo = skillProcessResult.skill.logoUrl,
                    skillId = skillProcessResult.skill.id
                )
            )
        }
        val widgetItem = skillProcessResult.widgetItem.get()
        return MainScreenSkillCardData(
            SkillInfoData(
                name = skillProcessResult.skill.name,
                logo = skillProcessResult.skill.logoUrl,
                skillId = skillProcessResult.skill.id
            ),
            MainScreenSkillCardData.SkillResponse(
                title = widgetItem.title,
                text = widgetItem.text,
                imageUrl = widgetItem.imageId?.let { getImageUrl(it) },
                buttons = getButtons(widgetItem),
                tapAction = getTapAction(widgetItem)
            )
        )
    }

    private fun getButtons(widgetGalleryMeta: WidgetGalleryMeta): List<MainScreenSkillCardData.Button> {
        widgetGalleryMeta.buttons ?: return emptyList()
        return widgetGalleryMeta.buttons.map {
            MainScreenSkillCardData.Button(
                title = it.title,
                payload = it.payload
            )
        }
    }

    private fun getTapAction(widgetGalleryMeta: WidgetGalleryMeta) =
        widgetGalleryMeta.tapAction?.let {
            MainScreenSkillCardData.TapAction(
                activationCommand = widgetGalleryMeta.tapAction.activationCommand,
                payload = widgetGalleryMeta.tapAction.payload
            )
        }

    private fun getImageUrl(imageId: String) =
        Image.getImageUrl(imageId, AvatarsNamespace.DIALOGS_SKILL_CARD, ImageAlias.ORIG, avatarsUrl)

    companion object {
        private const val avatarsUrl = "https://avatars.mds.yandex.net"
    }
}
