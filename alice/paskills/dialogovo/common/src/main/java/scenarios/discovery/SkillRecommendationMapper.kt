package ru.yandex.alice.paskill.dialogovo.scenarios.discovery

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.text.Phrases
import ru.yandex.alice.paskill.dialogovo.domain.ImageAlias
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo
import ru.yandex.alice.paskill.dialogovo.domain.SkillRecommendation
import ru.yandex.alice.paskill.dialogovo.utils.LogoUtils
import kotlin.random.Random

@Component
open class SkillRecommendationMapper(
    private val phrases: Phrases
) {
    fun mapToSkillRecommendationList(skillSequence: Sequence<SkillInfo>, request: MegaMindRequest<Any>) =
        mapToSkillRecommendationList(skillSequence.toList(), request)

    fun mapToSkillRecommendationList(skillInfoCollection: Collection<SkillInfo>, request: MegaMindRequest<Any>) =
        skillInfoCollection.map { skillInfo ->
            SkillRecommendation(
                skillInfo,
                getActivationExample(skillInfo, request.krandom),
                LogoUtils.makeLogo(skillInfo.logoUrl ?: "", ImageAlias.MOBILE_LOGO_X2)
            )
        }.toList()

    private fun getActivationExample(skillInfo: SkillInfo, r: Random): String {
        return skillInfo.activationExamples.randomOrNull(r)?.asText()
            ?: phrases["divs.discovery.recommendation.default.activation", listOf(skillInfo.name)]
    }
}
