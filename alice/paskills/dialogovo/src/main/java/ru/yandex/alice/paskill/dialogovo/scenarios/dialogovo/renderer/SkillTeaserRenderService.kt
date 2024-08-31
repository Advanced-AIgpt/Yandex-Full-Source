package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.renderer

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.domain.scenariodata.teasers.TeaserSkillCardData
import ru.yandex.alice.paskill.dialogovo.domain.AvatarsNamespace
import ru.yandex.alice.paskill.dialogovo.domain.Image
import ru.yandex.alice.paskill.dialogovo.domain.ImageAlias
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo
import ru.yandex.alice.paskill.dialogovo.external.v1.response.TeaserMeta

@Component
class SkillTeaserRenderService {

    fun getTeaserSkillCardData(skillInfo: SkillInfo, teaserMeta: TeaserMeta, teaserId: String): TeaserSkillCardData {
        return TeaserSkillCardData(
            skillInfo = getSkillInfoData(skillInfo),
            text = teaserMeta.text,
            title = teaserMeta.title,
            imageUrl = teaserMeta.imageId?.let { getImageUrl(it) },
            tapAction = teaserMeta.tapAction?.let { getTapAction(it) },
            teaserId = teaserId
        )
    }

    private fun getSkillInfoData(skillInfo: SkillInfo): TeaserSkillCardData.SkillInfoData {
        return TeaserSkillCardData.SkillInfoData(
            skillId = skillInfo.id,
            logo = skillInfo.logoUrl,
            name = skillInfo.name
        )
    }

    private fun getTapAction(tapAction: TeaserMeta.TapAction): TeaserSkillCardData.TapAction {
        return TeaserSkillCardData.TapAction(
            activationCommand = tapAction.activationCommand,
            payload = tapAction.payload
        )
    }

    private fun getImageUrl(imageId: String) =
        Image.getImageUrl(imageId, AvatarsNamespace.DIALOGS_SKILL_CARD, ImageAlias.ORIG, avatarsUrl)

    companion object {
        private const val avatarsUrl = "https://avatars.mds.yandex.net"
    }
}
