package ru.yandex.alice.paskill.dialogovo.providers.recommendation

import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillCategoryKey
import ru.yandex.alice.paskill.dialogovo.providers.skill.SkillProvider
import ru.yandex.alice.paskill.dialogovo.service.SurfaceChecker

internal class SkillByScoreAndCategoryRecommendationProvider(
    private val skillProvider: SkillProvider,
    surfaceChecker: SurfaceChecker,
    private val topN: Int,
    private val skillCategoryKey: SkillCategoryKey
) : BaseRandomSelectorRecommendationProvider(skillProvider, surfaceChecker) {

    public override fun getSkillIdList(request: MegaMindRequest<*>): List<String> {
        return skillProvider.getSkillsByCategory(skillCategoryKey)
            .sortedByDescending { skill -> skill.score }
            .take(topN)
            .map { skillInfo: SkillInfo -> skillInfo.id }
    }
}
