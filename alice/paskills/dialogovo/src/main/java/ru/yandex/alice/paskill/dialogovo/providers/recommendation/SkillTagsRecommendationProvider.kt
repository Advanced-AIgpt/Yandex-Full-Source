package ru.yandex.alice.paskill.dialogovo.providers.recommendation

import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillProvider
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillTagsKey
import ru.yandex.alice.paskill.dialogovo.service.SurfaceChecker

internal class SkillTagsRecommendationProvider(
    private val skillProvider: SkillProvider,
    surfaceChecker: SurfaceChecker,
    private val tagsKey: SkillTagsKey
) : BaseRandomSelectorRecommendationProvider(skillProvider, surfaceChecker) {

    public override fun getSkillIdList(request: MegaMindRequest<*>): List<String> {
        return skillProvider.getSkillsByTags(tagsKey)
            .map { skillInfo: SkillInfo -> skillInfo.id }
    }
}
